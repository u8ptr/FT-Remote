import QtQuick

Item {
    id: root
    property var values: []
    property color traceColor: "#d7f9ff"

    Rectangle { anchors.fill: parent; color: "#061923" }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = "#061923"
            ctx.fillRect(0, 0, width, height)
            ctx.strokeStyle = "#164357"
            ctx.lineWidth = 1
            for (let x = 0; x <= width; x += width / 10) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke() }
            for (let y = 0; y <= height; y += height / 5) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke() }
            if (!root.values || root.values.length < 2) return
            ctx.strokeStyle = root.traceColor
            ctx.lineWidth = 1.5
            ctx.beginPath()
            for (let i = 0; i < root.values.length; ++i) {
                const x = i * width / (root.values.length - 1)
                const y = height - Math.max(0, Math.min(1, root.values[i])) * height
                if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
            }
            ctx.stroke()
        }
    }

    onValuesChanged: canvas.requestPaint()
    Component.onCompleted: canvas.requestPaint()
}
