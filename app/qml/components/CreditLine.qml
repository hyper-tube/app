import QtQuick
import HtMusic

Item {
    id: root

    property var credits: []
    property string album: ""
    property string albumId: ""
    property string fallback: ""
    property real pixelSize: Theme.font.smaller
    property color colText: Theme.colInactive
    property color colHover: Theme.colOnSurface
    property bool interactive: true

    readonly property var segments: root.layout()
    readonly property int hoveredIndex: root.interactive && links.hoveredLink.length > 0
        ? Number(links.hoveredLink) : -1

    signal pageRequested(string browseId, string kind)

    function layout() {
        const pieces = [];
        const add = (text, browseId, kind) => pieces.push({ text: text, browseId: browseId, kind: kind });
        const names = root.credits ? Array.from(root.credits) : [];
        if (names.length === 0 && root.fallback.length > 0)
            add(root.fallback, "", "");
        for (let i = 0; i < names.length; ++i) {
            if (i > 0)
                add(", ", "", "");
            add(names[i].name, names[i].browseId, names[i].kind);
        }
        if (root.album.length > 0) {
            if (pieces.length > 0)
                add("  •  ", "", "");
            add(root.album, root.albumId, "album");
        }
        return pieces;
    }

    function escaped(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    function markup(anchored) {
        let text = "";
        for (let i = 0; i < root.segments.length; ++i) {
            const segment = root.segments[i];
            const piece = root.escaped(segment.text);
            if (anchored && segment.browseId.length > 0)
                text += "<a href=\"" + i + "\">" + piece + "</a>";
            else if (!anchored && i === root.hoveredIndex)
                text += "<u><font color=\"" + root.colHover + "\">" + piece + "</font></u>";
            else
                text += piece;
        }
        return text;
    }

    implicitHeight: label.implicitHeight

    StyledText {
        id: label

        width: parent.width
        text: root.markup(false)
        textFormat: Text.StyledText
        font.pixelSize: root.pixelSize
        color: root.colText
        elide: Text.ElideRight
    }

    StyledText {
        id: links

        width: parent.width
        opacity: 0
        enabled: root.interactive
        text: root.markup(true)
        textFormat: Text.StyledText
        font.pixelSize: root.pixelSize
        elide: Text.ElideRight
        Accessible.ignored: true

        HoverHandler {
            cursorShape: links.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
        }

        onLinkActivated: link => {
            const segment = root.segments[Number(link)];
            if (segment)
                root.pageRequested(segment.browseId, segment.kind);
        }
    }
}
