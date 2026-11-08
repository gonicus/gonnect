#include <QLoggingCategory>
#include <QRandomGenerator>
#include <QStringList>
#include <QXmlStreamReader>
#include <utility>
#include "SIPRemoteCc.h"
#include "SIPAccount.h"
#include "SIPAccountManager.h"
#include "PjSipTools.h"

#include <pjsua-lib/pjsua.h>

Q_LOGGING_CATEGORY(lcSIPRemoteCc, "gonnect.sip.cisco.remotecc")

namespace {

const char *const REMOTECC_REQUEST_SUBTYPE = "x-cisco-remotecc-request+xml";
const char *const REMOTECC_RESPONSE_SUBTYPE = "x-cisco-remotecc-response+xml";

struct ReferContext
{
    pj_pool_t *pool = nullptr;
    pjsua_acc_id accId = PJSUA_INVALID_ID;
    QString softKeyEvent;
    bool authRetried = false;
    bool authInitialized = false;
    pjsip_auth_clt_sess auth;

    bool sending = false;
    bool finishedWhileSending = false;
};

void destroyContext(ReferContext *ctx)
{
    if (ctx->authInitialized) {
        pjsip_auth_clt_deinit(&ctx->auth);
    }
    if (ctx->pool) {
        pj_pool_release(ctx->pool);
    }
    delete ctx;
}

void finishContext(ReferContext *ctx)
{
    if (ctx->sending) {
        ctx->finishedWhileSending = true;
    } else {
        destroyContext(ctx);
    }
}

void onReferResult(void *token, pjsip_event *e);

// Send the request - the context is owned by the transaction afterwards. Returns
// false if sending failed, the context is gone in that case.
bool sendWithContext(pjsip_tx_data *tdata, ReferContext *ctx)
{
    ctx->sending = true;
    ctx->finishedWhileSending = false;

    // tdata is released by pjsip on failure
    const pj_status_t status =
            pjsip_endpt_send_request(pjsua_get_pjsip_endpt(), tdata, -1, ctx, &onReferResult);

    ctx->sending = false;

    if (status != PJ_SUCCESS || ctx->finishedWhileSending) {
        destroyContext(ctx);
    }

    return status == PJ_SUCCESS;
}

QString accountName(pjsua_acc_id accId)
{
    const auto accounts = SIPAccountManager::instance().accounts();
    for (const auto account : accounts) {
        if (account->getId() == accId) {
            return account->id();
        }
    }

    return QString("#%1").arg(accId);
}

bool isRemoteCcType(const pjsip_media_type &type, QString *subtype = nullptr)
{
    if (pj_stricmp2(&type.type, "application") != 0) {
        return false;
    }

    if (pj_stricmp2(&type.subtype, REMOTECC_REQUEST_SUBTYPE) == 0
        || pj_stricmp2(&type.subtype, REMOTECC_RESPONSE_SUBTYPE) == 0) {
        if (subtype) {
            *subtype = PjSipTools::pjStringToQstring(type.subtype);
        }
        return true;
    }

    return false;
}

QString bodyText(const pjsip_msg_body *body)
{
    return QString::fromUtf8(static_cast<const char *>(body->data), static_cast<int>(body->len));
}

// Collect all remotecc XML documents of a body - either plain or a multipart bulk update
QList<std::pair<QString, QString>> remoteCcParts(pjsip_msg_body *body)
{
    QList<std::pair<QString, QString>> parts;
    if (!body) {
        return parts;
    }

    QString subtype;
    if (pj_stricmp2(&body->content_type.type, "multipart") == 0) {
        for (auto part = pjsip_multipart_get_first_part(body); part;
             part = pjsip_multipart_get_next_part(body, part)) {
            if (part->body && isRemoteCcType(part->body->content_type, &subtype)) {
                parts.push_back({ subtype, bodyText(part->body) });
            }
        }
    } else if (isRemoteCcType(body->content_type, &subtype)) {
        parts.push_back({ subtype, bodyText(body) });
    }

    return parts;
}

void logRemoteCcXml(const QString &account, const QString &subtype, const QString &xml)
{
    qCDebug(lcSIPRemoteCc).noquote() << "received" << subtype << "for account" << account << ":\n"
                                     << xml;

    QXmlStreamReader reader(xml);
    QStringList path;

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            path.push_back(reader.name().toString());
            if (path.size() == 2) {
                qCInfo(lcSIPRemoteCc) << "remotecc" << path.front() << "element" << path.back()
                                      << "for account" << account;
            }
        } else if (reader.isEndElement()) {
            if (!path.isEmpty()) {
                path.pop_back();
            }
        } else if (reader.isCharacters() && !reader.isWhitespace()) {
            if (path.size() == 3 && path.at(1) == "hlogupdate" && path.at(2) == "status") {
                qCInfo(lcSIPRemoteCc) << "hunt group login state of account" << account << "is"
                                      << reader.text().trimmed().toString();
            }
        }
    }

    if (reader.hasError()) {
        qCWarning(lcSIPRemoteCc) << "failed to parse remotecc XML:" << reader.errorString();
    }
}

pj_bool_t onRxRequest(pjsip_rx_data *rdata)
{
    pjsip_msg *msg = rdata->msg_info.msg;

    // Only out-of-dialog REFERs - in-dialog ones are taken by the dialog layer anyway
    if (pjsip_method_cmp(&msg->line.req.method, pjsip_get_refer_method()) != 0
        || (rdata->msg_info.to && rdata->msg_info.to->tag.slen > 0)) {
        return PJ_FALSE;
    }

    const auto parts = remoteCcParts(msg->body);
    if (parts.isEmpty()) {
        return PJ_FALSE;
    }

    const pj_status_t status = pjsip_endpt_respond(pjsua_get_pjsip_endpt(), nullptr, rdata, 202,
                                                   nullptr, nullptr, nullptr, nullptr);
    if (status != PJ_SUCCESS) {
        qCWarning(lcSIPRemoteCc) << "failed to answer remotecc REFER:" << status;
    }

    const QString account = accountName(pjsua_acc_find_for_incoming(rdata));
    for (const auto &part : parts) {
        logRemoteCcXml(account, part.first, part.second);
    }

    return PJ_TRUE;
}

pjsip_module s_module = [] {
    pjsip_module mod;
    pj_bzero(&mod, sizeof(mod));
    mod.name = pj_str(const_cast<char *>("mod-gonnect-cisco-remotecc"));
    mod.id = -1;
    // Before pjsua's application module, which would answer with 501
    mod.priority = PJSIP_MOD_PRIORITY_APPLICATION - 1;
    mod.on_rx_request = &onRxRequest;
    return mod;
}();

void onReferResult(void *token, pjsip_event *e)
{
    auto ctx = static_cast<ReferContext *>(token);

    if (e->type != PJSIP_EVENT_TSX_STATE) {
        return;
    }

    pjsip_transaction *tsx = e->body.tsx_state.tsx;
    const int code = tsx->status_code;
    if (code < 200) {
        return;
    }

    const bool hasRxMsg = e->body.tsx_state.type == PJSIP_EVENT_RX_MSG;
    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();

    // Authenticate once, if challenged
    if ((code == PJSIP_SC_UNAUTHORIZED || code == PJSIP_SC_PROXY_AUTHENTICATION_REQUIRED)
        && hasRxMsg && !ctx->authRetried) {
        ctx->authRetried = true;

        pjsua_acc_config cfg;
        pjsip_tx_data *tdata = nullptr;

        if (pjsua_acc_get_config(ctx->accId, ctx->pool, &cfg) == PJ_SUCCESS
            && (ctx->authInitialized =
                        pjsip_auth_clt_init(&ctx->auth, endpt, ctx->pool, 0) == PJ_SUCCESS)
            && pjsip_auth_clt_set_credentials(&ctx->auth, static_cast<int>(cfg.cred_count),
                                              cfg.cred_info)
                    == PJ_SUCCESS) {
            pjsip_auth_clt_set_prefs(&ctx->auth, &cfg.auth_pref);

            if (pjsip_auth_clt_reinit_req(&ctx->auth, e->body.tsx_state.src.rdata, tsx->last_tx,
                                          &tdata)
                == PJ_SUCCESS) {
                qCDebug(lcSIPRemoteCc)
                        << "resending remotecc" << ctx->softKeyEvent << "with authentication";
                const QString softKeyEvent = ctx->softKeyEvent;
                if (!sendWithContext(tdata, ctx)) {
                    qCWarning(lcSIPRemoteCc)
                            << "failed to resend remotecc" << softKeyEvent << "REFER";
                }
                return;
            }
        }

        qCWarning(lcSIPRemoteCc) << "failed to authenticate remotecc" << ctx->softKeyEvent
                                 << "REFER";
        finishContext(ctx);
        return;
    }

    const QString reason = PjSipTools::pjStringToQstring(tsx->status_text);
    if (code >= 200 && code < 300) {
        qCInfo(lcSIPRemoteCc) << "remotecc" << ctx->softKeyEvent << "for account"
                              << accountName(ctx->accId) << "accepted:" << code << reason;
    } else {
        qCWarning(lcSIPRemoteCc) << "remotecc" << ctx->softKeyEvent << "for account"
                                 << accountName(ctx->accId) << "failed:" << code << reason;
    }

    if (hasRxMsg) {
        const pjsip_msg_body *body = e->body.tsx_state.src.rdata->msg_info.msg->body;
        if (body && body->len) {
            qCDebug(lcSIPRemoteCc).noquote() << "remotecc response body:\n" << bodyText(body);
        }
    }

    finishContext(ctx);
}

void addHeader(pjsip_tx_data *tdata, const char *name, const QString &value)
{
    const QByteArray v = value.toUtf8();
    pj_str_t hName = pj_str(const_cast<char *>(name));
    pj_str_t hValue;
    pj_strset(&hValue, const_cast<char *>(v.constData()), v.size());

    // Name and value are duplicated into the pool
    auto hdr = pjsip_generic_string_hdr_create(tdata->pool, &hName, &hValue);
    pjsip_msg_add_hdr(tdata->msg, reinterpret_cast<pjsip_hdr *>(hdr));
}

QString softKeyEventBody(const QString &softKeyEvent)
{
    // Modeled after what Cisco Jabber (Cisco-CSF) sends
    return QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<x-cisco-remotecc-request>\n"
                   "  <softkeyeventmsg>\n"
                   "    <softkeyevent>%1</softkeyevent>\n"
                   "    <dialogid>\n"
                   "      <callid></callid>\n"
                   "      <localtag></localtag>\n"
                   "      <remotetag></remotetag>\n"
                   "    </dialogid>\n"
                   "    <linenumber>0</linenumber>\n"
                   "    <participantnum>0</participantnum>\n"
                   "    <consultdialogid>\n"
                   "      <callid></callid>\n"
                   "      <localtag></localtag>\n"
                   "      <remotetag></remotetag>\n"
                   "    </consultdialogid>\n"
                   "    <state>false</state>\n"
                   "    <joindialogid>\n"
                   "      <callid></callid>\n"
                   "      <localtag></localtag>\n"
                   "      <remotetag></remotetag>\n"
                   "    </joindialogid>\n"
                   "    <eventdata>\n"
                   "      <invocationtype>explicit</invocationtype>\n"
                   "    </eventdata>\n"
                   "    <userdata></userdata>\n"
                   "    <softkeyid>0</softkeyid>\n"
                   "    <applicationid>0</applicationid>\n"
                   "  </softkeyeventmsg>\n"
                   "</x-cisco-remotecc-request>")
            .arg(softKeyEvent.toHtmlEscaped());
}

} // namespace

bool SIPRemoteCc::registerModule()
{
    if (s_module.id != -1) {
        return true;
    }

    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();
    if (!endpt) {
        return false;
    }

    const pj_status_t status = pjsip_endpt_register_module(endpt, &s_module);
    if (status != PJ_SUCCESS) {
        qCCritical(lcSIPRemoteCc) << "failed to register remotecc module:" << status;
        return false;
    }

    return true;
}

bool SIPRemoteCc::sendSoftKeyEvent(SIPAccount *account, const QString &softKeyEvent)
{
    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();
    const pjsua_acc_id accId = account ? account->getId() : PJSUA_INVALID_ID;

    if (!endpt || accId == PJSUA_INVALID_ID || !pjsua_acc_is_valid(accId)) {
        qCWarning(lcSIPRemoteCc) << "no valid account to send remotecc" << softKeyEvent;
        return false;
    }

    // Like Jabber: address the PBX domain
    std::string target = account->ciscoRemoteCcTarget().toStdString();
    pj_str_t pjTarget = pj_str(target.data());

    pjsip_tx_data *tdata = nullptr;
    pj_status_t status =
            pjsua_acc_create_request(accId, pjsip_get_refer_method(), &pjTarget, &tdata);
    if (status != PJ_SUCCESS) {
        qCWarning(lcSIPRemoteCc) << "failed to create remotecc REFER to" << target.c_str();
        return false;
    }

    // pjsua_acc_create_request() does not add a Contact
    pj_str_t contact;
    status = pjsua_acc_create_uac_contact(tdata->pool, &contact, accId, &pjTarget);
    if (status != PJ_SUCCESS) {
        qCWarning(lcSIPRemoteCc) << "failed to create Contact for remotecc REFER";
        pjsip_tx_data_dec_ref(tdata);
        return false;
    }

    pjsua_acc_info accInfo;
    pjsua_acc_get_info(accId, &accInfo);

    const QString contentId =
            QString("%1@%2")
                    .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'))
                    .arg(account->domain());

    addHeader(tdata, "Contact",
              PjSipTools::pjStringToQstring(contact) + account->ciscoContactParams());
    addHeader(tdata, "Accept", "application/x-cisco-remotecc-response+xml");
    addHeader(tdata, "Expires", "3600");
    addHeader(tdata, "Require", "norefersub");
    addHeader(tdata, "Referred-By",
              QString("<%1>").arg(PjSipTools::pjStringToQstring(accInfo.acc_uri)));
    addHeader(tdata, "Refer-To", "cid:" + contentId);
    addHeader(tdata, "Content-Id", QString("<%1>").arg(contentId));
    addHeader(tdata, "Content-Disposition", "session;handling=required");

    const QByteArray xml = softKeyEventBody(softKeyEvent).toUtf8();
    pj_str_t type = pj_str(const_cast<char *>("application"));
    pj_str_t subtype = pj_str(const_cast<char *>(REMOTECC_REQUEST_SUBTYPE));
    pj_str_t text;
    pj_strset(&text, const_cast<char *>(xml.constData()), xml.size());
    tdata->msg->body = pjsip_msg_body_create(tdata->pool, &type, &subtype, &text);

    auto ctx = new ReferContext;
    ctx->pool = pjsua_pool_create("remotecc", 1024, 1024);
    ctx->accId = accId;
    ctx->softKeyEvent = softKeyEvent;

    if (!sendWithContext(tdata, ctx)) {
        qCWarning(lcSIPRemoteCc) << "failed to send remotecc" << softKeyEvent << "REFER";
        return false;
    }

    qCInfo(lcSIPRemoteCc) << "sent remotecc" << softKeyEvent << "for account" << account->id()
                          << "to" << target.c_str();
    return true;
}
