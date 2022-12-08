#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include <wx/rawbmp.h>

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
			a.label = wxString(shape->textData);
			a.x = shape->bounds[0];
			a.y = shape->bounds[1];
			a.fontSize = shape->fontSize;
			a.family = wxString(shape->fontFamily);
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

		if( !m_bitmap.IsOk() || render_width != m_bitmap.GetWidth() || render_height != m_bitmap.GetHeight() )
		{
			m_bitmap.Create(render_width, render_height, 32);

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
		}

		if(m_bitmap.IsOk())
		{
			dc.DrawBitmap(m_bitmap, 0, 0, true);
			//m_bitmap.SaveFile( "d:\\_nano_svg_test.png", wxBITMAP_TYPE_PNG );
		}

        // show text labels on top of the bitmap
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
	} //if( m_svg_image!=NULL )
};
