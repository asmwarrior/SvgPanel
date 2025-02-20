#ifndef _SVG_PANEL_H
#define _SVG_PANEL_H

#define wxCOLOR_FROM_NSVG(x) wxColour( x&0xff, (x>>8)&0xff, (x>>16)&0xff, (x>>24)&0xff )

// for showing the text label on the SVGPanel
struct TextLabel
{
	wxString id;        /// id string
	wxString label;     /// the text content of the label
	float x;            /// x coordinates of label
	float y;            /// y coordinates of label
	wxString family;    /// font name string, e.g. Arial, Courier New
	float fontSize;     /// font size
	wxColor fillColor; /// the filled color

	// Constructor with default fillColor set to red
	TextLabel()
		: fillColor(*wxRED)  // Default color is red
	{}
};

struct NSVGimage;
class SVGPanel : public wxPanel
{
public:
	SVGPanel(wxWindow* parent, int id, const wxString& svg_filename = wxEmptyString);
	~SVGPanel();
	void OnPaint(wxPaintEvent& event);

	void LoadSVG(const wxString& filename);

private:
	NSVGimage* m_svg_image;
	wxBitmap m_bitmap;

	// for showing the text label on the SVGPanel
	std::vector<TextLabel> m_TextLabels;
	float m_Scale; /// user scaled from m_svg_image to m_bitmap

	DECLARE_EVENT_TABLE()
};

#endif
