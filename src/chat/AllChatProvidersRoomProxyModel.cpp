#include "AllChatProvidersRoomProxyModel.h"
#include "ChatConnectorManager.h"
#include "IChatProvider.h"
#include "ChatRoomModel.h"

#include <algorithm>

AllChatProvidersRoomProxyModel::AllChatProvidersRoomProxyModel(QObject *parent)
    : QConcatenateTablesProxyModel{ parent }
{
    auto &manager = ChatConnectorManager::instance();

    connect(&manager, &ChatConnectorManager::chatConnectorsChanged, this,
            &AllChatProvidersRoomProxyModel::populate);

    // The provider list might already be populated before this model has been created
    populate();
}

void AllChatProvidersRoomProxyModel::populate()
{
    auto providers = ChatConnectorManager::instance().chatConnectors();

    std::sort(providers.begin(), providers.end(),
              [](const IChatProvider *a, const IChatProvider *b) -> bool {
                  return a->id() < b->id();
              });

    const auto currModels = sourceModels();
    const bool sameProviders =
            currModels.length() == providers.length()
            && std::equal(
                    currModels.begin(), currModels.end(), providers.begin(),
                    [](const QAbstractItemModel *model, const IChatProvider *provider) -> bool {
                        return model->property("chatProvider").value<IChatProvider *>() == provider;
                    });

    if (sameProviders) {
        return;
    }

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