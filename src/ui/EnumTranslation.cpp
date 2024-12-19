#include "EnumTranslation.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcEnumTranslation, "gonnect.app.EnumTranslation")

EnumTranslation::EnumTranslation(QObject *parent) : QObject{ parent } { }

QString EnumTranslation::sipStatusCode(const pjsip_status_code statusCode) const
{
    switch (statusCode) {
    case PJSIP_SC_NULL:
        return "Null";
    case PJSIP_SC_TRYING:
        return tr("Trying");
    case PJSIP_SC_RINGING:
        return tr("Ringing");
    case PJSIP_SC_CALL_BEING_FORWARDED:
        return tr("Call being forwarded");
    case PJSIP_SC_QUEUED:
        return tr("Queued");
    case PJSIP_SC_PROGRESS:
        return tr("Progress");
    case PJSIP_SC_EARLY_DIALOG_TERMINATED:
        return "Early dialog terminated";
    case PJSIP_SC_OK:
        return tr("Ok");
    case PJSIP_SC_ACCEPTED:
        return tr("Accepted");
    case PJSIP_SC_NO_NOTIFICATION:
        return "No notification";
    case PJSIP_SC_MULTIPLE_CHOICES:
        return "Multiple choices";
    case PJSIP_SC_MOVED_PERMANENTLY:
        return "Moved permanently";
    case PJSIP_SC_MOVED_TEMPORARILY:
        return "Moved temporarily";
    case PJSIP_SC_USE_PROXY:
        return "Use proxy";
    case PJSIP_SC_ALTERNATIVE_SERVICE:
        return "Alternative service";
    case PJSIP_SC_BAD_REQUEST:
        return "Bad request";
    case PJSIP_SC_UNAUTHORIZED:
        return tr("Unauthorized");
    case PJSIP_SC_PAYMENT_REQUIRED:
        return "Payment required";
    case PJSIP_SC_FORBIDDEN:
        return tr("Rejected");
    case PJSIP_SC_NOT_FOUND:
        return tr("Not found");
    case PJSIP_SC_METHOD_NOT_ALLOWED:
        return "Method not allowed";
    case PJSIP_SC_NOT_ACCEPTABLE:
        return "Not acceptable";
    case PJSIP_SC_PROXY_AUTHENTICATION_REQUIRED:
        return tr("Proxy authentication required");
    case PJSIP_SC_REQUEST_TIMEOUT:
        return tr("Request timeout");
    case PJSIP_SC_CONFLICT:
        return "Conflict";
    case PJSIP_SC_GONE:
        return "Gone";
    case PJSIP_SC_LENGTH_REQUIRED:
        return "Length required";
    case PJSIP_SC_CONDITIONAL_REQUEST_FAILED:
        return "Conditional request failed";
    case PJSIP_SC_REQUEST_ENTITY_TOO_LARGE:
        return "Request entity too large";
    case PJSIP_SC_REQUEST_URI_TOO_LONG:
        return "Request uri too long";
    case PJSIP_SC_UNSUPPORTED_MEDIA_TYPE:
        return "Unsupported media type";
    case PJSIP_SC_UNSUPPORTED_URI_SCHEME:
        return "Unsupported uri scheme";
    case PJSIP_SC_UNKNOWN_RESOURCE_PRIORITY:
        return "Unknown resource priority";
    case PJSIP_SC_BAD_EXTENSION:
        return "Bad extension";
    case PJSIP_SC_EXTENSION_REQUIRED:
        return "Extension required";
    case PJSIP_SC_SESSION_TIMER_TOO_SMALL:
        return "Session timer too small";
    case PJSIP_SC_INTERVAL_TOO_BRIEF:
        return "Interval too brief";
    case PJSIP_SC_BAD_LOCATION_INFORMATION:
        return "Bad location information";
    case PJSIP_SC_USE_IDENTITY_HEADER:
        return "Use identity header";
    case PJSIP_SC_PROVIDE_REFERRER_HEADER:
        return "Provide referrer header";
    case PJSIP_SC_FLOW_FAILED:
        return "Flow failed";
    case PJSIP_SC_ANONIMITY_DISALLOWED:
        return "Anonimity disallowed";
    case PJSIP_SC_BAD_IDENTITY_INFO:
        return "Bad identity info";
    case PJSIP_SC_UNSUPPORTED_CERTIFICATE:
        return "Unsupported certificate";
    case PJSIP_SC_INVALID_IDENTITY_HEADER:
        return "Invalid identity header";
    case PJSIP_SC_FIRST_HOP_LACKS_OUTBOUND_SUPPORT:
        return "First hop lacks outbound support";
    case PJSIP_SC_MAX_BREADTH_EXCEEDED:
        return "Max breadth exceeded";
    case PJSIP_SC_BAD_INFO_PACKAGE:
        return "Bad info package";
    case PJSIP_SC_CONSENT_NEEDED:
        return "Consent needed";
    case PJSIP_SC_TEMPORARILY_UNAVAILABLE:
        return tr("Temporarily unavailable");
    case PJSIP_SC_CALL_TSX_DOES_NOT_EXIST:
        return "Call tsx does not exist";
    case PJSIP_SC_LOOP_DETECTED:
        return "Loop detected";
    case PJSIP_SC_TOO_MANY_HOPS:
        return "Too many hops";
    case PJSIP_SC_ADDRESS_INCOMPLETE:
        return "Address incomplete";
    case PJSIP_AC_AMBIGUOUS:
        return tr("Ambiguous");
    case PJSIP_SC_BUSY_HERE:
        return tr("Busy here");
    case PJSIP_SC_REQUEST_TERMINATED:
        return tr("Request terminated");
    case PJSIP_SC_NOT_ACCEPTABLE_HERE:
        return tr("Not acceptable here");
    case PJSIP_SC_BAD_EVENT:
        return "Bad event";
    case PJSIP_SC_REQUEST_UPDATED:
        return "Request updated";
    case PJSIP_SC_REQUEST_PENDING:
        return "Request pending";
    case PJSIP_SC_UNDECIPHERABLE:
        return "Undecipherable";
    case PJSIP_SC_SECURITY_AGREEMENT_NEEDED:
        return "Security agreement needed";
    case PJSIP_SC_INTERNAL_SERVER_ERROR:
        return tr("Internal server error");
    case PJSIP_SC_NOT_IMPLEMENTED:
        return tr("Not implemented");
    case PJSIP_SC_BAD_GATEWAY:
        return tr("Bad gateway");
    case PJSIP_SC_SERVICE_UNAVAILABLE:
        return tr("Service unavailable");
    case PJSIP_SC_SERVER_TIMEOUT:
        return tr("Server timeout");
    case PJSIP_SC_VERSION_NOT_SUPPORTED:
        return "Version not supported";
    case PJSIP_SC_MESSAGE_TOO_LARGE:
        return "Message too large";
    case PJSIP_SC_PUSH_NOTIFICATION_SERVICE_NOT_SUPPORTED:
        return "Push notification service not supported";
    case PJSIP_SC_PRECONDITION_FAILURE:
        return "Precondition failure";
    case PJSIP_SC_BUSY_EVERYWHERE:
        return tr("Busy everywhere");
    case PJSIP_SC_DECLINE:
        return tr("Decline");
    case PJSIP_SC_DOES_NOT_EXIST_ANYWHERE:
        return tr("Does not exist anywhere");
    case PJSIP_SC_NOT_ACCEPTABLE_ANYWHERE:
        return tr("Not acceptable anywhere");
    case PJSIP_SC_UNWANTED:
        return tr("Unwanted");
    case PJSIP_SC_REJECTED:
        return tr("Rejected");
    case PJSIP_SC__force_32bit:
        return "Force 32bit";
    }

    qCWarning(lcEnumTranslation) << "Unknown SIP code:" << statusCode;
    return tr("Unknown");
}

QString EnumTranslation::sipStatusCode(const int statusCode) const
{
    return sipStatusCode(static_cast<pjsip_status_code>(statusCode));
}

QString EnumTranslation::numberType(const Contact::NumberType numberType) const
{

    switch (numberType) {
    case Contact::NumberType::Unknown:
        return tr("Unknown");
    case Contact::NumberType::Commercial:
        return tr("Commercial");
    case Contact::NumberType::Home:
        return tr("Home");
    case Contact::NumberType::Mobile:
        return tr("Mobile");
    }

    qCWarning(lcEnumTranslation) << "Unknown number type:" << numberType;
    return tr("Unknown");
}

QString EnumTranslation::callType(const CallHistoryItem::Type callType) const
{
    switch (callType) {

    case CallHistoryItem::Type::Incoming:
        return tr("Incoming");
    case CallHistoryItem::Type::Outgoing:
        return tr("Outgoing");
    case CallHistoryItem::Type::IncomingBlocked:
        return tr("Blocked");
    }

    qCWarning(lcEnumTranslation) << "Unknown call type:" << callType;
    return tr("Unknown");
}
