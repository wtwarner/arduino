// Draws the 1024×1024 app icon: a nixie tube beside an IV-25 style VFD tube.
// Usage: swift tools/make_icon.swift BLERemote/Assets.xcassets/AppIcon.appiconset/AppIcon.png

import AppKit

let S: CGFloat = 1024
let outPath = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "AppIcon.png"
let cs = CGColorSpace(name: CGColorSpace.sRGB)!

// Opaque bitmap (App Store icons must not have alpha), top-left origin.
let ctx = CGContext(data: nil, width: Int(S), height: Int(S), bitsPerComponent: 8, bytesPerRow: 0,
                    space: cs, bitmapInfo: CGImageAlphaInfo.noneSkipLast.rawValue)!
ctx.translateBy(x: 0, y: S)
ctx.scaleBy(x: 1, y: -1)
NSGraphicsContext.current = NSGraphicsContext(cgContext: ctx, flipped: true)

func rgb(_ hex: UInt32, _ a: CGFloat = 1) -> CGColor {
    CGColor(srgbRed: CGFloat((hex >> 16) & 0xFF) / 255, green: CGFloat((hex >> 8) & 0xFF) / 255,
            blue: CGFloat(hex & 0xFF) / 255, alpha: a)
}

func nsrgb(_ hex: UInt32, _ a: CGFloat = 1) -> NSColor { NSColor(cgColor: rgb(hex, a))! }

func gradient(_ colors: [CGColor], _ locations: [CGFloat]? = nil) -> CGGradient {
    CGGradient(colorsSpace: cs, colors: colors as CFArray, locations: locations)!
}

func radialGlow(_ c: CGPoint, _ r: CGFloat, _ hex: UInt32, _ a: CGFloat) {
    ctx.drawRadialGradient(gradient([rgb(hex, a), rgb(hex, 0)]), startCenter: c, startRadius: 0,
                           endCenter: c, endRadius: r, options: [])
}

func withClip(_ path: CGPath, _ body: () -> Void) {
    ctx.saveGState()
    ctx.addPath(path)
    ctx.clip()
    body()
    ctx.restoreGState()
}

/// Glass envelope: round top, gently rounded bottom.
func tubePath(x: CGFloat, top: CGFloat, w: CGFloat, bottom: CGFloat, topR: CGFloat, botR: CGFloat) -> CGPath {
    let p = CGMutablePath()
    let l = x, r = x + w
    p.move(to: CGPoint(x: l, y: bottom - botR))
    p.addLine(to: CGPoint(x: l, y: top + topR))
    p.addArc(tangent1End: CGPoint(x: l, y: top), tangent2End: CGPoint(x: l + topR, y: top), radius: topR)
    p.addArc(tangent1End: CGPoint(x: r, y: top), tangent2End: CGPoint(x: r, y: top + topR), radius: topR)
    p.addLine(to: CGPoint(x: r, y: bottom - botR))
    p.addArc(tangent1End: CGPoint(x: r, y: bottom), tangent2End: CGPoint(x: r - botR, y: bottom), radius: botR)
    p.addArc(tangent1End: CGPoint(x: l, y: bottom), tangent2End: CGPoint(x: l, y: bottom - botR), radius: botR)
    p.closeSubpath()
    return p
}

func drawGlass(_ path: CGPath, x: CGFloat, w: CGFloat, top: CGFloat, bottom: CGFloat, tint: UInt32) {
    // Glass body: brighter at the edges, like a cylinder catching light.
    withClip(path) {
        ctx.drawLinearGradient(
            gradient([rgb(tint, 0.16), rgb(tint, 0.03), rgb(tint, 0.02), rgb(tint, 0.12)], [0, 0.3, 0.7, 1]),
            start: CGPoint(x: x, y: 0), end: CGPoint(x: x + w, y: 0), options: [])
    }
}

func drawGlassHighlights(_ path: CGPath, x: CGFloat, w: CGFloat, top: CGFloat, bottom: CGFloat) {
    ctx.addPath(path)
    ctx.setStrokeColor(rgb(0xFFFFFF, 0.32))
    ctx.setLineWidth(5)
    ctx.strokePath()

    // Specular streak down the left side.
    let streak = CGPath(roundedRect: CGRect(x: x + w * 0.11, y: top + w * 0.35, width: w * 0.07,
                                            height: bottom - top - w * 0.55),
                        cornerWidth: w * 0.035, cornerHeight: w * 0.035, transform: nil)
    withClip(streak) {
        ctx.drawLinearGradient(gradient([rgb(0xFFFFFF, 0.0), rgb(0xFFFFFF, 0.35), rgb(0xFFFFFF, 0.05)]),
                               start: CGPoint(x: 0, y: top), end: CGPoint(x: 0, y: bottom), options: [])
    }
}

func drawBase(x: CGFloat, w: CGFloat, top: CGFloat, h: CGFloat) {
    let base = CGPath(roundedRect: CGRect(x: x, y: top, width: w, height: h),
                      cornerWidth: 18, cornerHeight: 18, transform: nil)
    withClip(base) {
        ctx.drawLinearGradient(gradient([rgb(0x3A3E4A), rgb(0x16181F)]),
                               start: CGPoint(x: 0, y: top), end: CGPoint(x: 0, y: top + h), options: [])
    }
    ctx.addPath(base)
    ctx.setStrokeColor(rgb(0xFFFFFF, 0.12))
    ctx.setLineWidth(3)
    ctx.strokePath()
}

// MARK: Background

ctx.drawRadialGradient(gradient([rgb(0x1E2333), rgb(0x07080D)]),
                       startCenter: CGPoint(x: S / 2, y: S * 0.42), startRadius: 0,
                       endCenter: CGPoint(x: S / 2, y: S * 0.42), endRadius: S * 0.78,
                       options: [.drawsAfterEndLocation])
// The tube pair spans x 191…790; shift it onto the icon's center line.
ctx.translateBy(x: 22, y: 0)
radialGlow(CGPoint(x: 365, y: 470), 360, 0xFF7A1A, 0.30)
radialGlow(CGPoint(x: 700, y: 450), 300, 0x30FFC8, 0.22)

// Floor reflections and contact shadows.
ctx.saveGState()
ctx.scaleBy(x: 1, y: 0.18)
radialGlow(CGPoint(x: 365, y: 905 / 0.18), 230, 0xFF7A1A, 0.35)
radialGlow(CGPoint(x: 695, y: 905 / 0.18), 160, 0x30FFC8, 0.30)
ctx.restoreGState()

// MARK: Nixie tube

let nx: CGFloat = 205, nw: CGFloat = 320, nTop: CGFloat = 150, nBottom: CGFloat = 810
let nixie = tubePath(x: nx, top: nTop, w: nw, bottom: nBottom, topR: nw / 2, botR: 36)
let ncx = nx + nw / 2, ncy: CGFloat = 505

drawGlass(nixie, x: nx, w: nw, top: nTop, bottom: nBottom, tint: 0xFFD8B0)
withClip(nixie) {
    // Unlit cathodes stacked behind the lit one.
    let font = NSFont.systemFont(ofSize: 430, weight: .thin)
    for (d, dx) in [("8", -6.0), ("0", 5.0), ("3", -2.0)] {
        let s = NSAttributedString(string: d, attributes: [.font: font, .foregroundColor: nsrgb(0xC08060, 0.10)])
        let sz = s.size()
        s.draw(at: CGPoint(x: ncx - sz.width / 2 + dx, y: ncy - sz.height / 2))
    }

    // Lit digit: wide orange halo, tighter glow, then a hot core.
    for (blur, hex, coreHex) in [(80.0, 0xFF5A00, 0xFF8A2A), (60.0, 0xFF5A00, 0xFF8A2A), (26.0, 0xFF7A1A, 0xFFA040), (6.0, 0xFFB060, 0xFFE2B8)] as [(CGFloat, UInt32, UInt32)] {
        let shadow = NSShadow()
        shadow.shadowBlurRadius = blur
        shadow.shadowColor = nsrgb(hex)
        shadow.shadowOffset = .zero
        let s = NSAttributedString(string: "5", attributes: [.font: font, .foregroundColor: nsrgb(coreHex),
                                                             .shadow: shadow])
        let sz = s.size()
        s.draw(at: CGPoint(x: ncx - sz.width / 2, y: ncy - sz.height / 2))
    }

    // Hexagonal anode mesh in front of the digits.
    ctx.setStrokeColor(rgb(0xFFB080, 0.13))
    ctx.setLineWidth(2)
    let r: CGFloat = 17, hw = r * sqrt(3)
    for row in 0..<34 {
        for col in 0..<13 {
            let cx = nx - hw + CGFloat(col) * hw + (row % 2 == 0 ? 0 : hw / 2)
            let cy = nTop + CGFloat(row) * r * 1.5
            let hex = CGMutablePath()
            for k in 0..<6 {
                let a = CGFloat(k) * .pi / 3 + .pi / 6
                let p = CGPoint(x: cx + r * cos(a), y: cy + r * sin(a))
                k == 0 ? hex.move(to: p) : hex.addLine(to: p)
            }
            hex.closeSubpath()
            ctx.addPath(hex)
        }
    }
    ctx.strokePath()
}
drawGlassHighlights(nixie, x: nx, w: nw, top: nTop, bottom: nBottom)
// Exhaust tip on top.
let tip = CGPath(roundedRect: CGRect(x: ncx - 16, y: nTop - 26, width: 32, height: 34),
                 cornerWidth: 14, cornerHeight: 14, transform: nil)
withClip(tip) {
    ctx.drawLinearGradient(gradient([rgb(0xFFFFFF, 0.35), rgb(0xFFFFFF, 0.08)]),
                           start: CGPoint(x: ncx - 16, y: 0), end: CGPoint(x: ncx + 16, y: 0), options: [])
}
drawBase(x: nx - 14, w: nw + 28, top: nBottom - 18, h: 88)

// MARK: VFD tube (IV-25: a slim tube with a column of segments)

let vx: CGFloat = 612, vw: CGFloat = 166, vTop: CGFloat = 118, vBottom: CGFloat = 810
let vfd = tubePath(x: vx, top: vTop, w: vw, bottom: vBottom, topR: vw / 2, botR: 30)
let vcx = vx + vw / 2

drawGlass(vfd, x: vx, w: vw, top: vTop, bottom: vBottom, tint: 0xB8FFE8)
withClip(vfd) {
    let segW: CGFloat = 74, segH: CGFloat = 50, gap: CGFloat = 16, count = 8
    let firstTop: CGFloat = 225
    let lit = 5 // bottom five glowing, like a bar graph

    for i in 0..<count {
        let y = firstTop + CGFloat(i) * (segH + gap)
        let rect = CGRect(x: vcx - segW / 2, y: y, width: segW, height: segH)
        let seg = CGPath(roundedRect: rect, cornerWidth: 12, cornerHeight: 12, transform: nil)
        if i >= count - lit {
            ctx.saveGState()
            ctx.setShadow(offset: .zero, blur: 46, color: rgb(0x20FFC0))
            ctx.addPath(seg)
            ctx.setFillColor(rgb(0x5CFFD2))
            ctx.fillPath()
            ctx.setShadow(offset: .zero, blur: 14, color: rgb(0x8BFFE2))
            ctx.addPath(seg)
            ctx.fillPath()
            ctx.restoreGState()
            let core = CGPath(roundedRect: rect.insetBy(dx: 14, dy: 13), cornerWidth: 8, cornerHeight: 8,
                              transform: nil)
            ctx.addPath(core)
            ctx.setFillColor(rgb(0xE4FFF7, 0.85))
            ctx.fillPath()
        } else {
            ctx.addPath(seg)
            ctx.setFillColor(rgb(0x5CFFD2, 0.09))
            ctx.fillPath()
        }
    }

    // Filament wires stretched in front of the segments.
    ctx.setStrokeColor(rgb(0xFFFFFF, 0.18))
    ctx.setLineWidth(1.5)
    for dx in [-22.0, 0.0, 22.0] as [CGFloat] {
        ctx.move(to: CGPoint(x: vcx + dx, y: vTop + 70))
        ctx.addLine(to: CGPoint(x: vcx + dx, y: vBottom - 50))
    }
    ctx.strokePath()

    // Fine control grid over the segments.
    ctx.setStrokeColor(rgb(0xC8FFF0, 0.10))
    ctx.setLineWidth(1)
    var gy: CGFloat = 215
    while gy < 760 {
        ctx.move(to: CGPoint(x: vcx - 46, y: gy))
        ctx.addLine(to: CGPoint(x: vcx + 46, y: gy))
        gy += 9
    }
    ctx.strokePath()
}
drawGlassHighlights(vfd, x: vx, w: vw, top: vTop, bottom: vBottom)
drawBase(x: vx - 12, w: vw + 24, top: vBottom - 18, h: 88)

// MARK: Write PNG

let image = ctx.makeImage()!
let rep = NSBitmapImageRep(cgImage: image)
let png = rep.representation(using: .png, properties: [:])!
try! png.write(to: URL(fileURLWithPath: outPath))
print("Wrote \(outPath)")
