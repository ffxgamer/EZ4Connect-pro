import AppKit

let size = NSSize(width: 680, height: 340)
let image = NSImage(size: size)
image.lockFocus()
NSColor.white.setFill()
NSBezierPath(rect: NSRect(origin: .zero, size: size)).fill()

func draw(_ text: String, y: CGFloat, size: CGFloat, font: NSFont, color: NSColor) {
    let style = NSMutableParagraphStyle()
    style.alignment = .center
    let attrs: [NSAttributedString.Key: Any] = [
        .font: font, .foregroundColor: color, .paragraphStyle: style
    ]
    (text as NSString).draw(in: NSRect(x: 20, y: y, width: 640, height: size), withAttributes: attrs)
}

draw("将 EZ4Connect Pro 拖入“应用程序”", y: 298, size: 24,
     font: NSFont.systemFont(ofSize: 16, weight: .medium), color: .darkGray)
draw("首次打开如被系统拦截，可在终端执行：", y: 69, size: 20,
     font: NSFont.systemFont(ofSize: 13), color: .darkGray)
draw("xattr -dr com.apple.quarantine \"/Applications/EZ4Connect Pro.app\"", y: 43, size: 20,
     font: NSFont.monospacedSystemFont(ofSize: 12, weight: .regular), color: .black)
image.unlockFocus()
guard CommandLine.arguments.count == 2,
      let tiff = image.tiffRepresentation,
      let bitmap = NSBitmapImageRep(data: tiff),
      let png = bitmap.representation(using: .png, properties: [:]) else { exit(1) }
try png.write(to: URL(fileURLWithPath: CommandLine.arguments[1]))
