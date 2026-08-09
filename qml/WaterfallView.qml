import QtQuick

Item {
    id: root
    property var row: []
    property var rows: []
    property int maxRows: 320

    Rectangle { anchors.fill: parent; color: "#001125" }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = "#001125"
            ctx.fillRect(0, 0, width, height)
            if (!root.rows || root.rows.length === 0) return
            const rowHeight = height / root.maxRows
            const start = Math.max(0, root.rows.length - root.maxRows)
            for (let r = start; r < root.rows.length; ++r) {
                const values = root.rows[r]
                const y = height - (root.rows.length - r) * rowHeight
                const cellWidth = width / Math.max(1, values.length)
                for (let i = 0; i < values.length; ++i) {
                    const v = Math.max(0, Math.min(1, values[i]))
                    const hue = 0.67 - v * 0.67
                    ctx.fillStyle = Qt.hsla(hue, 0.95, 0.25 + v * 0.38, 1)
                    ctx.fillRect(i * cellWidth, y, cellWidth + 1, rowHeight + 1)
                }
            }
        }
    }

    onRowChanged: {
        if (!row || row.length === 0) return
        const next = rows.slice()
        next.push(row)
        if (next.length > maxRows) next.shift()
        rows = next
        canvas.requestPaint()
    }
}
