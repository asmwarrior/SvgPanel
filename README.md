# SvgPanel
A wxPanel which can show svg image and simple text labels.

The code contains two parts:
1, original file `nanosvg.h` and `nanosvgrast.h` are from [memononen/nanosvg: Simple stupid SVG parser](https://github.com/memononen/nanosvg)
2, original file `svg_panel.h` and `svg_panel.cpp` are from doublemax's svg panel sources in [wxWidgets Discussion Forum - Index page](https://forums.wxwidgets.org/), see this post for details [Re: Use SVG Icons with wxDataViewCtrl (and others)](https://forums.wxwidgets.org/viewtopic.php?p=184852#p184852)

Note that currently,  [memononen/nanosvg: Simple stupid SVG parser](https://github.com/memononen/nanosvg) does not support parsing the `Text` field in `svg` file. I have modifed `nanosvg.h` to support parsing the `Text` field in the SVG file. I mainly followed commits in this pull request: [Adds basic text parsing by jamislike · Pull Request #94 · memononen/nanosvg](https://github.com/memononen/nanosvg/pull/94), but with some extra code changes, Also, I have merged from the latest master code of nanosvg.

The `svg_panel.h` and `svg_panel.cpp` files are modifed to extract the `Text` field and show them by wxWidgets' `DrawText` function. So, basically, the rasterizer from nanosvg is used to generated the background wxBitmap image, and later text labels were drawn on top of the wxBitmap image.

Here is the screen shot of a svg file `drawing.svg` shown in the panel compared with shown in the [Inkscape editor](https://inkscape.org/). You can also open the svg file in the web browser directly for a comparsion.

 ![svg image shown in panel vs Inkscape](./svg_show_panel_vs_inkscape.png "svg image shown in panel vs Inkscape")

# How to use the code

Just copy/drag the 4 source files(`nanosvg.h`, `nanosvgrast.h`, `svg_panel.h` and `svg_panel.cpp`) to your project file, and construct a `SVGPanel` window where you needed.

# Note

## anchor position of the text

```
    <text
       xml:space="preserve"
       style="font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:7.05556px;line-height:1.25;font-family:Arial;-inkscape-font-specification:'Arial, Normal';font-variant-ligatures:normal;font-variant-caps:normal;font-variant-numeric:normal;font-variant-east-asian:normal;text-align:center;letter-spacing:0px;word-spacing:0px;writing-mode:lr-tb;text-anchor:start;stroke-width:0.264583"
       x="32.977989"
       y="11.358852"
       id="text1-7"><tspan
         sodipodi:role="line"
         id="tspan1-7"
         style="font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:7.05556px;font-family:Arial;-inkscape-font-specification:'Arial, Normal';font-variant-ligatures:normal;font-variant-caps:normal;font-variant-numeric:normal;font-variant-east-asian:normal;fill:#ff00ff;stroke-width:0.264583"
         x="32.977989"
         y="11.358852">wxWidgets SVG Panel demo</tspan></text>
```

In the above section of svg file, you can see that `text-anchor:start` is used, currently, the text parser can't handle the `text-anchor:middle`, if you have `text-anchor:middle` in your svg file, the text is rendered as `text-anchor:start`.

## tspan section inside tspan

Currently, the parser can't handle the `tspan` section inside another `tspan` section(This usually happens you have different colors for a single `text` section. Normally, it can handle a `tspan` section inside a `text` section.
