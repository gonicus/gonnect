#pragma once

#include <QLoggingCategory>

// Internal assertion macros shared by the IpcDispatcher translation units
// (IpcDispatcher.cpp and IpcDispatcherResponses.cpp). Not intended for use elsewhere.

Q_DECLARE_LOGGING_CATEGORY(lcIpcDispatcher)

#define GONNECT_ASSERT_HAS_VERIFICATION                                               \
    if (!hasDeviceVerification()) {                                                   \
        qCCritical(lcIpcDispatcher) << "Received verification content although "      \
                                       "hasDeviceVerification() is false - ignoring"; \
    }
#define GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS                                                \
    if (!m_isInVerificationProcess) {                                                            \
        qCCritical(lcIpcDispatcher) << "Expected to be in verification process, but it is not."; \
        return;                                                                                  \
    }
#define GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS                                            \
    if (m_isInVerificationProcess) {                                                             \
        qCCritical(lcIpcDispatcher) << "Expected not to be in verification process, but it is."; \
        return;                                                                                  \
    }

#define GONNECT_ASSERT_VERIFICATION_PROCESS(verificationFlowId)                           \
    if (m_verificationFlowId != verificationFlowId) {                                     \
        qCCritical(lcIpcDispatcher)                                                       \
                << "Received verification content for process" << verificationFlowId      \
                << "but am currently in process" << m_verificationFlowId << "- ignoring"; \
        return;                                                                           \
    }

#define GONNECT_ASSERT(condition, failMessage)                             \
    if (!(condition)) {                                                    \
        qCCritical(lcIpcDispatcher) << "Assertion failed:" << failMessage; \
        return;                                                            \
    }
