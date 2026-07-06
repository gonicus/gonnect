#include "IpcProtoConversion.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcIpcProtoConversion, "gonnect.app.chat.IpcProtoConversion")

using namespace de::gonicus::gonnect;

IChatRoom::UserRoomState userRoomStateConv(const UserRoomStateGadget::UserRoomState state)
{
    switch (state) {

    case UserRoomStateGadget::UserRoomState::Unjoined:
        return IChatRoom::UserRoomState::Unjoined;
    case UserRoomStateGadget::UserRoomState::Joined:
        return IChatRoom::UserRoomState::Joined;
    case UserRoomStateGadget::UserRoomState::Invited:
        return IChatRoom::UserRoomState::Invited;
    case UserRoomStateGadget::UserRoomState::Knocked:
        return IChatRoom::UserRoomState::Knocked;
    case UserRoomStateGadget::UserRoomState::Banned:
        return IChatRoom::UserRoomState::Banned;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown state:" << state;
    qFatal("Unknown state");
    Q_UNREACHABLE();
}

ChatUser::PresenceState presenceStateConv(const PresenceStateGadget::PresenceState state)
{
    switch (state) {

    case PresenceStateGadget::PresenceState::Unknown:
        return ChatUser::PresenceState::Unknown;
    case PresenceStateGadget::PresenceState::Offline:
        return ChatUser::PresenceState::Offline;
    case PresenceStateGadget::PresenceState::Away:
        return ChatUser::PresenceState::Away;
    case PresenceStateGadget::PresenceState::Online:
        return ChatUser::PresenceState::Online;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown state:" << state;
    qFatal("Unknown state");
    Q_UNREACHABLE();
}

CrossSigningSecret::CrossSigningMethod
crossSigningMethodConv(const CrossSigningMethodGadget::CrossSigningMethod method)
{
    switch (method) {

    case CrossSigningMethodGadget::CrossSigningMethod::SasString:
        return CrossSigningSecret::CrossSigningMethod::SasString;
    case CrossSigningMethodGadget::CrossSigningMethod::SasSymbol:
        return CrossSigningSecret::CrossSigningMethod::SasSymbol;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown cross signing method:" << method;
    qFatal("Unknown method");
    Q_UNREACHABLE();
}

CrossSigningMethodGadget::CrossSigningMethod
crossSigningMethodReConv(const CrossSigningSecret::CrossSigningMethod method)
{
    switch (method) {

    case CrossSigningSecret::CrossSigningMethod::SasString:
        return CrossSigningMethodGadget::CrossSigningMethod::SasString;
    case CrossSigningSecret::CrossSigningMethod::SasSymbol:
        return CrossSigningMethodGadget::CrossSigningMethod::SasSymbol;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown cross signing method:" << method;
    qFatal("Unknown method");
    Q_UNREACHABLE();
}

NotificationSettingGadget::NotificationSetting
notificationSettingIpcToProto(NotificationSetting::Setting setting)
{
    using Setting = NotificationSettingGadget::NotificationSetting;

    switch (setting) {

    case NotificationSetting::Setting::All:
        return Setting::AllMessages;
    case NotificationSetting::Setting::MentionsAndKeywords:
        return Setting::MentionsAndKeywordsOnly;
    case NotificationSetting::Setting::Mute:
        return Setting::Mute;
    case NotificationSetting::Setting::None:
        // Never happens as "None" is unknown in proto definition
        break;
    }

    // Fallback to make compiler happy
    return Setting::Mute;
}

NotificationSetting::Setting
notificationSettingProtoToIpc(NotificationSettingGadget::NotificationSetting setting)
{
    switch (setting) {

    case NotificationSettingGadget::NotificationSetting::AllMessages:
        return NotificationSetting::Setting::All;
    case NotificationSettingGadget::NotificationSetting::MentionsAndKeywordsOnly:
        return NotificationSetting::Setting::MentionsAndKeywords;
    case NotificationSettingGadget::NotificationSetting::Mute:
        return NotificationSetting::Setting::Mute;
    }

    return NotificationSetting::Setting::None;
}

::RoomSettings roomSettingsProtoToIpc(const de::gonicus::gonnect::RoomSettings &protoSettings)
{
    return { protoSettings.hasNotificationSetting()
                     ? notificationSettingProtoToIpc(protoSettings.notificationSetting())
                     : NotificationSetting::Setting::None };
}

IChatRoom::JoinRule joinRuleGrpcToGonnect(const RoomJoinRuleGadget::RoomJoinRule grpcRule)
{
    using JoinRule = IChatRoom::JoinRule;

    switch (grpcRule) {

    case RoomJoinRuleGadget::RoomJoinRule::Invite:
        return JoinRule::Invite;
    case RoomJoinRuleGadget::RoomJoinRule::Knock:
        return JoinRule::Knock;
    case RoomJoinRuleGadget::RoomJoinRule::Public:
        return JoinRule::Public;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown room join enum value:" << grpcRule;
    return JoinRule::Unknown;
}

RoomJoinRuleGadget::RoomJoinRule joinRuleGonnectToGrpc(const IChatRoom::JoinRule joinRule)
{
    using Rule = RoomJoinRuleGadget::RoomJoinRule;

    switch (joinRule) {
    case IChatRoom::JoinRule::Invite:
        return Rule::Invite;
    case IChatRoom::JoinRule::Knock:
        return Rule::Knock;
    case IChatRoom::JoinRule::Public:
        return Rule::Public;
    case IChatRoom::JoinRule::Unknown:
        qCCritical(lcIpcProtoConversion)
                << "Enum value 'Unknown' cannot be converted - defaulting to 'Invite'";
        return Rule::Invite;
    }

    qCCritical(lcIpcProtoConversion)
            << "Unknown enum value" << joinRule << "- defaulting to 'Invite'";
    return Rule::Invite;
}

IChatRoom::LeaveReason leaveReasonGrpcToGonnect(const RoomLeftEvent::RoomLeaveReason leaveReason)
{
    using LeaveReason = IChatRoom::LeaveReason;

    switch (leaveReason) {

    case RoomLeftEvent::RoomLeaveReason::User:
        return LeaveReason::User;
    case RoomLeftEvent::RoomLeaveReason::Kicked:
        return LeaveReason::Kicked;
    case RoomLeftEvent::RoomLeaveReason::Banned:
        return LeaveReason::Banned;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown leave reason enum value:" << leaveReason;
    return LeaveReason::Unknown;
}

RoomLeftEvent::RoomLeaveReason leaveReasonGonnectToGrpc(const IChatRoom::LeaveReason leaveReason)
{
    using Reason = RoomLeftEvent::RoomLeaveReason;

    switch (leaveReason) {

    case IChatRoom::LeaveReason::User:
        return Reason::User;
    case IChatRoom::LeaveReason::Kicked:
        return Reason::Kicked;
    case IChatRoom::LeaveReason::Banned:
        return Reason::Banned;
    case IChatRoom::LeaveReason::Unknown:
        qCCritical(lcIpcProtoConversion)
                << "The enum value 'Unknown' cannot be converted - defaulting to 'User'";
        return Reason::User;
    }

    qCCritical(lcIpcProtoConversion)
            << "Unknown enum value" << leaveReason << "- defaulting to 'User'";
    return Reason::User;
}

ChatMessageContentUserStateChange::State
userStateGrpcToGonnect(MessageContentMembershipChange::MembershipChange change)
{
    using State = ChatMessageContentUserStateChange::State;

    switch (change) {

    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Joined:
        return State::Joined;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Left:
        return State::Left;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Invited:
        return State::Invited;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Knocked:
        return State::Knocked;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Banned:
        return State::Banned;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Unbanned:
        return State::Unbanned;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Kicked:
        return State::Kicked;
    }

    qCCritical(lcIpcProtoConversion) << "Unknown membership change enum value:" << change;
    return State::Unknown;
}

IChatRoom::Permissions
roomPermissionsGrpcToGonnect(const de::gonicus::gonnect::RoomPermissions &permissions)
{
    IChatRoom::Permissions p;

    if (permissions.canEdit()) {
        p |= IChatRoom::Permission::CanEdit;
    }
    if (permissions.canInvite()) {
        p |= IChatRoom::Permission::CanInvite;
    }
    if (permissions.canKick()) {
        p |= IChatRoom::Permission::CanKick;
    }
    if (permissions.canBan()) {
        p |= IChatRoom::Permission::CanBan;
    }

    return p;
}
