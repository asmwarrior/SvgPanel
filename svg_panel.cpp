#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include <wx/rawbmp.h>
#include <wx/colour.h>

#define NANOSVG_ALL_COLOR_KEYWORDS  // Include full list of color keywords.

#define NANOSVG_IMPLEMENTATION      // Expands implementation
#include "nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include "svg_panel.h"

typedef wxAlphaPixelData PixelData;

BEGIN_EVENT_TABLE(SVGPanel, wxPanel)
	EVT_PAINT(SVGPanel::OnPaint)
END_EVENT_TABLE()

// Function to convert NanoSVG color to wxColor
wxColor ConvertNSVGColorToWxColor(unsigned int nsvgColor)
{
	unsigned char a = (nsvgColor >> 24) & 0xFF;  // Extract alpha
	unsigned char r = (nsvgColor >> 16) & 0xFF;  // Extract red
	unsigned char g = (nsvgColor >> 8) & 0xFF;   // Extract green
	unsigned char b = nsvgColor & 0xFF;          // Extract blue

	return wxColor(r, g, b, a);  // Create wxColor with RGBA
}

SVGPanel::SVGPanel(wxWindow* parent, int id, const wxString& svg_filename)
	: wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
	  m_svg_image(NULL)
{
	if(!svg_filename.IsEmpty())
		m_svg_image = nsvgParseFromFile(svg_filename.c_str(), "px", 96.0f);

	// Use...
	for(NSVGshape* shape = m_svg_image->shapes; shape != NULL; shape = shape->next)
	{
		if(shape->isText == 1)
		{
			// printf("text size = %f, family = %s, content = %s, x = %f, y = %f\n", shape->fontSize, shape->fontFamily, shape->textData, shape->bounds[0], shape->bounds[1]);
			TextLabel a;
			a.id = wxString(shape->id);
			a.label = wxString(shape->textData);
			a.x = shape->bounds[0];
			a.y = shape->bounds[1];
			a.fontSize = shape->fontSize;
			a.family = wxString(shape->fontFamily);
			if (shape->fill.type == NSVG_PAINT_COLOR)
			{
				a.fillColor = ConvertNSVGColorToWxColor(shape->fill.color);
			}
			if (a.label.IsEmpty())
				continue;
			m_TextLabels.push_back(a);
		}
	}

	SetBackgroundStyle(wxBG_STYLE_PAINT);
};


SVGPanel::~SVGPanel()
{
	if(m_svg_image != NULL)
		nsvgDelete(m_svg_image);
};

void SVGPanel::LoadSVG(const wxString& filename)
{
	if(m_svg_image != NULL)
		nsvgDelete(m_svg_image);

	m_svg_image = nsvgParseFromFile(filename.c_str(), "px", 96.0f);
	m_bitmap = wxNullBitmap;
	Refresh();
};

void SVGPanel::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	dc.SetBrush(*wxWHITE_BRUSH);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawRectangle(0, 0, GetClientSize().x, GetClientSize().y);

	if(m_svg_image != NULL)
	{
		double svg_ratio = m_svg_image->width / m_svg_image->height;
		double panel_ratio = (double)GetClientSize().x / GetClientSize().y;

		int render_width = 0;
		int render_height = 0;
		if(svg_ratio > panel_ratio)
		{
			render_width = GetClientSize().x;
			render_height = svg_ratio * GetClientSize().y;
		}
		else
		{
			render_width = (double)GetClientSize().y * svg_ratio;
			render_height = GetClientSize().y;
		}

		if(render_width == 0 || render_height == 0)
			return;

		if(!m_bitmap.IsOk() || render_width != m_bitmap.GetWidth() || render_height != m_bitmap.GetHeight())
		{
			m_bitmap.Create(render_width, render_height, 32);
			m_bitmap.UseAlpha(true);

			NSVGrasterizer* rast = nsvgCreateRasterizer();
			unsigned char* image_buffer = (unsigned char*)malloc(render_width * render_height * 4);

			float scale = (float)render_width / m_svg_image->width;
			m_Scale = scale;

			//wxLogDebug("rasterizing image %d x %d (scale=%.2f)", render_width, render_height, scale);
			//printf("rasterizing image %d x %d (scale=%.2f)", render_width, render_height, scale);
			nsvgRasterize(rast, m_svg_image, 0, 0, scale, image_buffer, render_width, render_height, render_width * 4);

			PixelData bmdata(m_bitmap);
			PixelData::Iterator dst(bmdata);

			unsigned char* source_data = image_buffer;
			for(int y = 0; y < m_bitmap.GetHeight(); y++)
			{
				dst.MoveTo(bmdata, 0, y);
				for(int x = 0; x < m_bitmap.GetWidth(); x++)
				{
					const unsigned char alpha = source_data[3];
					dst.Blue() = source_data[2] * alpha / 255;
					dst.Green() = source_data[1] * alpha / 255;
					dst.Red() = source_data[0] * alpha / 255;
					dst.Alpha() = alpha;
					dst++;
					source_data += 4;
				}
			}
			nsvgDeleteRasterizer(rast);
			free(image_buffer);
			// draw text on top
			wxGraphicsContext* gc = wxGraphicsContext::Create(m_bitmap);
			gc->SetAntialiasMode(wxANTIALIAS_NONE);
			for(TextLabel& a : m_TextLabels)
			{
				if (a.label.IsEmpty())
					continue;
				// The coordinates refer to the top-left corner of the rectangle bounding the string.
				// See GetTextExtent() for how to get the dimensions of a text string,
				// which can be used to position the text more precisely

				// for svg text, its x,y is defined as: x: The x coordinate of the starting point of the text baseline.
				// so generally the left bottom corner of the first character

				//wxFont font(a.fontSize*4*g_Scale, wxROMAN, wxNORMAL, wxLIGHT, false, _T("Times New Roman"));

				// 2022-04-10 point size is in pt unit, we have to convert it to px unit
				// One point(pt) is the equivalent of 1.333(3) pixels(px)

				wxFont font(wxFontInfo(a.fontSize * m_Scale / 1.3333).FaceName(a.family));

				// enable the smooth font by default
				// font rendering issue when using C::B under windows remote desktop
				// https://forums.codeblocks.org/index.php/topic,25146.msg171484.html#msg171484

				wxString nativeDesc = font.GetNativeFontInfoDesc();
				int index = 0;
				for(size_t pos = 0, start = 0; pos <= nativeDesc.length();)
				{
					pos = nativeDesc.find(";", start);
					index++;
					if(index == 14)
					{
						// enable the cleartype option of the font
						nativeDesc.replace(start, pos - start, "5");
						bool result = font.SetNativeFontInfo(nativeDesc);
						break;
					}
					start = pos + 1;
				}

				wxGraphicsFont gfont = gc->CreateFont(font, a.fillColor);
				gc->SetFont(gfont);

				// size calculation
				wxDouble  w, h;
				wxDouble descent;
				gc->GetTextExtent(a.label, &w, &h, &descent);

				gc->DrawText(a.label, a.x * m_Scale, a.y * m_Scale - h + descent);
			} //for (TextLabel &a : m_TextLabels)
			delete gc;
		}

		if(m_bitmap.IsOk())
		{
			dc.DrawBitmap(m_bitmap, 0, 0, true);
			//m_bitmap.SaveFile( "d:\\_nano_svg_test.png", wxBITMAP_TYPE_PNG );
		}

#if 0 // disable drawing the text labels on DC, we just need to build it on bitmap
		// show text labels on top of the bitmap
		for(TextLabel& a : m_TextLabels)
		{
			// The coordinates refer to the top-left corner of the rectangle bounding the string.
			// See GetTextExtent() for how to get the dimensions of a text string,
			// which can be used to position the text more precisely

			// for svg text, its x,y is defined as: x: The x coordinate of the starting point of the text baseline.
			// so generally the left bottom corner of the first character

			//wxFont font(a.fontSize*4*g_Scale, wxROMAN, wxNORMAL, wxLIGHT, false, _T("Times New Roman"));

			// 2022-04-10 point size is in pt unit, we have to convert it to px unit
			// One point(pt) is the equivalent of 1.333(3) pixels(px)

			wxFont font(wxFontInfo(a.fontSize * m_Scale / 1.3333).FaceName(a.family).AntiAliased(true));
			dc.SetFont(font);

			// size calculation
			wxCoord  w, h;
			wxCoord descent;
			dc.GetTextExtent(a.label, &w, &h, &descent);

			dc.DrawText(a.label, a.x * m_Scale, a.y * m_Scale - h + descent);
		} //for (TextLabel &a : m_TextLabels)
#endif
	} //if( m_svg_image!=NULL )
};
