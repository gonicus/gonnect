#pragma once

#include "chat.qpb.h"
#include "IChatRoom.h"
#include "ChatUser.h"
#include "ChatMessageContentUserStateChange.h"
#include "CrossSigningSecret.h"
#include "NotificationSetting.h"

// Pure conversion helpers between the protobuf/grpc types of the chat IPC protocol and their
// gonnect counterparts.

/// Convert UserRoomState value from grpc to IChatRoom definition.
IChatRoom::UserRoomState
userRoomStateConv(de::gonicus::gonnect::UserRoomStateGadget::UserRoomState state);

/// Convert PresenceState value from grpc to ChatUser definition.
ChatUser::PresenceState
presenceStateConv(de::gonicus::gonnect::PresenceStateGadget::PresenceState state);

/// Convert CrossSigningMethod method from grpc to CrossSigningSecret definition.
CrossSigningSecret::CrossSigningMethod crossSigningMethodConv(
        de::gonicus::gonnect::CrossSigningMethodGadget::CrossSigningMethod method);

/// Convert CrossSigningMethod method from CrossSigningSecret definition to grpc.
de::gonicus::gonnect::CrossSigningMethodGadget::CrossSigningMethod
crossSigningMethodReConv(CrossSigningSecret::CrossSigningMethod method);

de::gonicus::gonnect::NotificationSettingGadget::NotificationSetting
notificationSettingIpcToProto(NotificationSetting::Setting setting);
NotificationSetting::Setting notificationSettingProtoToIpc(
        de::gonicus::gonnect::NotificationSettingGadget::NotificationSetting setting);

::RoomSettings roomSettingsProtoToIpc(const de::gonicus::gonnect::RoomSettings &protoSettings);

IChatRoom::JoinRule
joinRuleGrpcToGonnect(de::gonicus::gonnect::RoomJoinRuleGadget::RoomJoinRule grpcRule);
de::gonicus::gonnect::RoomJoinRuleGadget::RoomJoinRule
joinRuleGonnectToGrpc(IChatRoom::JoinRule joinRule);

IChatRoom::LeaveReason
leaveReasonGrpcToGonnect(de::gonicus::gonnect::RoomLeftEvent::RoomLeaveReason leaveReason);
de::gonicus::gonnect::RoomLeftEvent::RoomLeaveReason
leaveReasonGonnectToGrpc(IChatRoom::LeaveReason leaveReason);

ChatMessageContentUserStateChange::State userStateGrpcToGonnect(
        de::gonicus::gonnect::MessageContentMembershipChange::MembershipChange change);

IChatRoom::Permissions
roomPermissionsGrpcToGonnect(const de::gonicus::gonnect::RoomPermissions &permissions);
