#include "AllChatProvidersRoomProxyModel.h"
#include "ChatConnectorManager.h"
#include "IChatProvider.h"
#include "ChatRoomModel.h"

AllChatProvidersRoomProxyModel::AllChatProvidersRoomProxyModel(QObject *parent)
    : QConcatenateTablesProxyModel{ parent }
{
    auto &manager = ChatConnectorManager::instance();

    connect(&manager, &ChatConnectorManager::chatConnectorsChanged, [this]() {
        auto providers = ChatConnectorManager::instance().chatConnectors();
        const auto currModels = sourceModels();

        if (providers.length() != currModels.length()) {
            std::sort(providers.begin(), providers.end(),
                      [](const IChatProvider *a, const IChatProvider *b) -> bool {
                          return a->id() < b->id();
                      });

            for (auto *model : currModels) {
                removeSourceModel(model);
                model->deleteLater();
            }

            for (auto *provider : std::as_const(providers)) {
                auto *roomModel = new ChatRoomModel(this);
                roomModel->setProperty("chatProvider", QVariant::fromValue(provider));
                addSourceModel(roomModel);
            }
        }
    });
}
