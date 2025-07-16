import QtQuick
import base

QtObject {
    id: control

    readonly property alias selectedItem: internal.selectedItem
    readonly property alias selectedSubItem: internal.selectedSubItem
    readonly property alias selectedInternally: internal.selectedInternally

    property int columns: 3

    onColumnsChanged: () => internal.reset()

    signal verticallyOutOfBounds

    function addContainer(containerItem : SearchResultCategory, position : int) {
        const newArr = internal.resultContainersWithPosition.concat([{ containerItem, position }])
        newArr.sort((a, b) => a.position - b.position)
        internal.resultContainersWithPosition = newArr
    }

    function reset() {
        internal.row = -1
        internal.col = -1
        internal.selectedItem = null
        internal.resetCache()
    }

    function setExternallySelected(item : SearchResultItem, subItem : Item) {
        internal.selectedInternally = false
        const pos = internal.getPositionOf(item)
        internal.row = pos.row
        internal.col = pos.col
        internal.selectedItem = item
        internal.selectedSubItem = subItem ?? null
    }

    function keyDown() {
        internal.selectedInternally = true

        if (internal.selectedItem && internal.selectedSubItem) {
            // Sub navigation inside one result item
            const highlightables = internal.collectHighlightableSubItems(internal.selectedItem)
            const highlightedIndex = highlightables.indexOf(internal.selectedSubItem)
            if (highlightedIndex >= 0 && highlightedIndex + 1 < highlightables.length) {
                internal.selectedSubItem = highlightables[highlightedIndex + 1]
                return
            }
        }
        internal.selectedSubItem = null

        if (internal.col < 0) {
            internal.col = 0
        }

        const resultContainers = internal.containerCache()

        for (let c = internal.col; c >= 0; --c) {
            const item = internal.getItemAt(internal.row + 1, c)
            if (item) {
                if (!item.canBeHighlighted) {
                    const highlightables = internal.collectHighlightableSubItems(item)
                    if (highlightables.length) {
                        internal.selectedSubItem = highlightables[0]
                    }
                }

                internal.row = internal.row + 1
                internal.col = c
                internal.selectedItem = item
                return
            }
        }

        internal.selectedItem = null
        internal.row = -1
        internal.col = -1
        control.verticallyOutOfBounds()
    }

    function keyUp() {
        internal.selectedInternally = true

        if (internal.selectedItem && internal.selectedSubItem) {
            // Sub navigation inside one result item
            const highlightables = internal.collectHighlightableSubItems(internal.selectedItem)

            const highlightedIndex = highlightables.indexOf(internal.selectedSubItem)
            if (highlightedIndex > 0) {
                internal.selectedSubItem = highlightables[highlightedIndex - 1]
                return
            }
        }

        internal.selectedSubItem = null

        if (internal.col < 0) {
            internal.col = 0
        }

        const resultContainers = internal.containerCache()

        if (internal.row < 0) {
            // Default to last one in the given column

            for (let containerIndex = resultContainers.length - 1; containerIndex >= 0; --containerIndex) {
                const containerRow = resultContainers[containerIndex]
                const rowChildren = internal.collectSearchResultItems(containerRow)
                if (rowChildren.length) {
                    internal.selectedItem = rowChildren[Math.floor(rowChildren.length / control.columns) * control.columns]
                    const pos = internal.getPositionOf(internal.selectedItem)
                    internal.col = pos.col
                    internal.row = pos.row

                    if (!internal.selectedItem.canBeHighlighted) {
                        const highlightables = internal.collectHighlightableSubItems(internal.selectedItem)
                        if (highlightables.length) {
                            internal.selectedSubItem = highlightables[highlightables.length - 1]
                        }
                    }
                    return
                }
            }
            internal.col = 0
            return
        }

        if (internal.row === 0) {
            internal.selectedItem = null
            internal.row = -1
            internal.col = -1
            control.verticallyOutOfBounds()
            return
        }

        for (let c = internal.col; c >= 0; --c) {
            const item = internal.getItemAt(internal.row - 1, c)
            if (item) {
                if (!item.canBeHighlighted) {
                    const highlightables = internal.collectHighlightableSubItems(item)
                    if (highlightables.length) {
                        internal.selectedSubItem = highlightables[highlightables.length - 1]
                    }
                }

                internal.row = internal.row - 1
                internal.col = c
                internal.selectedItem = item
                return
            }
        }

        internal.selectedItem = null
        internal.row = -1
        internal.col = -1
        control.verticallyOutOfBounds()
    }

    function keyLeft() {
        internal.selectedInternally = true

        if (internal.col > 0) {
            internal.col = internal.col - 1
            internal.selectedItem = internal.getItemAt(internal.row, internal.col)
            internal.selectedSubItem = null

            if (!internal.selectedItem.canBeHighlighted) {
                const highlightables = internal.collectHighlightableSubItems(internal.selectedItem)
                if (highlightables.length) {
                    internal.selectedSubItem = highlightables[0]
                }
            }

            return
        }

        internal.selectedSubItem = null

        for (let i = control.columns - 1; i >= 0; --i) {
            const item = internal.getItemAt(internal.row, i)

            if (item) {
                internal.col = i
                internal.selectedItem = item
                internal.selectedSubItem = null

                if (!item.canBeHighlighted) {
                    const highlightables = internal.collectHighlightableSubItems(item)
                    if (highlightables.length) {
                        internal.selectedSubItem = highlightables[0]
                    }
                }
                return
            }
        }
    }

    function keyRight() {
        internal.selectedInternally = true

        let newCol = (internal.col + 1) % control.columns
        let item = internal.getItemAt(internal.row, newCol)

        if (!item) {
            // No item to the right - wrap around and take first in row
            newCol = 0
            item = internal.getItemAt(internal.row, newCol)
        }
        if (item) {
            internal.selectedItem = item
            internal.selectedSubItem = null
            internal.col = newCol

            if (!item.canBeHighlighted) {
                const highlightables = internal.collectHighlightableSubItems(item)
                if (highlightables.length) {
                    internal.selectedSubItem = highlightables[0]
                }
            }
        }

    }

    readonly property QtObject _internalObject: QtObject {
        id: internal


        property Item selectedItem: null
        property Item selectedSubItem: null
        property bool selectedInternally: false
        property int row: -1
        property int col: -1

        property variant resultContainersWithPosition: []
        property list<SearchResultCategory> containerCacheList: []
        property bool containerCacheBuilt: false
        property variant resultItemCache: ({})
        property variant subItemCache: ({})

        function resetCache() {
            internal.resultItemCache = {}
            internal.subItemCache = {}
            internal.containerCacheList = []
            internal.containerCacheBuilt = false
        }

        function containerCache() : list<SearchResultCategory> {
            if (!internal.containerCacheBuilt) {
                internal.containerCacheBuilt = true
                internal.containerCacheList = internal.resultContainersWithPosition.filter(({containerItem}) => containerItem.visible).map(item => item.containerItem)
            }
            return internal.containerCacheList
        }

        function getPositionOf(item : variant) : variant {

            let realRow = 0
            const resultContainers = internal.containerCache()

            for (let containerIndex = 0; containerIndex <= resultContainers.length; ++containerIndex) {
                const rowChildren = internal.collectSearchResultItems(resultContainers[containerIndex])
                let realCol = 0

                for (let i = 0; i < rowChildren.length; ++i) {
                    if (rowChildren[i] === item) {
                        return { row: realRow, col: realCol }
                    }

                    ++realCol
                    if (realCol === control.columns) {
                        ++realRow
                        realCol = 0
                    }
                }
                ++realRow
            }

            return { row: -1, col: -1 }
        }

        function getItemAt(row : int, col : int) : variant {
            if (row < 0 || col < 0 || col >= control.columns) {
                return null
            }

            let realRow = 0
            const resultContainers = internal.containerCache()

            for (let containerIndex = 0; containerIndex <= resultContainers.length; ++containerIndex) {
                const rowChildren = internal.collectSearchResultItems(resultContainers[containerIndex])
                const rowsPerContainer = Math.ceil(rowChildren.length / control.columns)

                if (!rowsPerContainer) {
                    // Ignore empty lines
                    continue
                }

                if (realRow <= row && row < realRow + rowsPerContainer) {
                    return rowChildren[(row - realRow) * 3 + col]
                }
                realRow += rowsPerContainer
            }

            return null
        }

        function collectHighlightableSubItems(item : variant) : variant {

            if (!internal.subItemCache.hasOwnProperty(item)) {
                let result = []

                for (const child of item.children) {
                    result = result.concat(internal._collectHighlightableSubItemsImpl(child))
                }

                internal.subItemCache[item] = result
            }
            return internal.subItemCache[item]
        }

        function _collectHighlightableSubItemsImpl(item : variant) : variant {
            let result = []

            if (item.visible && item.hasOwnProperty("highlighted")) {
                result.push(item)
            }

            for (const child of item.children) {
                result = result.concat(internal._collectHighlightableSubItemsImpl(child))
            }

            return result
        }

        function collectSearchResultItems(item : variant) : variant {
            if (!item) {
                return []
            }

            if (!internal.resultItemCache.hasOwnProperty(item)) {
                internal.resultItemCache[item] = internal._collectSearchResultItemsImpl(item)
            }
            return internal.resultItemCache[item]
        }

        function _collectSearchResultItemsImpl(item : variant) : variant {
            let result = []

            if (item.visible && item instanceof SearchResultItem) {
                result.push(item)
            }

            for (const child of item.children) {
                result = result.concat(internal.collectSearchResultItems(child))
            }

            return result
        }
    }
}
