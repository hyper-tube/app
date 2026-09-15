pragma Singleton

import QtQuick

QtObject {
    id: root

    signal posted(string message, bool error)

    function show(message, error = false) {
        if (message.length > 0)
            root.posted(message, error);
    }
}
