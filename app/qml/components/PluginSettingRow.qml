import QtQuick
import HtMusic

SettingRow {
    id: root

    required property Plugin plugin
    required property var descriptor

    readonly property bool picker: root.descriptor.kind === "select"

    title: root.descriptor.label
    caption: root.descriptor.caption
    controlWidth: root.picker ? 246 : 52

    ToggleSwitch {
        visible: !root.picker
        height: visible ? implicitHeight : 0
        checked: root.plugin.values[root.descriptor.key] === true
        Accessible.name: root.descriptor.label
        onToggled: value => root.plugin.setValue(root.descriptor.key, value)
    }

    SelectField {
        visible: root.picker
        width: 246
        height: visible ? 42 : 0
        options: root.descriptor.options
        value: root.picker ? String(root.plugin.values[root.descriptor.key]) : ""
        onPicked: chosen => root.plugin.setValue(root.descriptor.key, chosen)
    }
}
