#pragma once
#include <string>

struct LabelTemplateSettings
{
	int xBarcode = 0;
	int yBarcode = -15;
	int xText = 5;
	int yTextBase = -15;
	int xLogo = 165;
	int yLogo = 75;
	int maxTextWidth = 300; // max width for text wrapping
	std::string labelFormat = "DYMO_99012"; // default format
	bool showBrandingText = true; // default to show branding text
	std::string brandingText = "Branding Text"; // default branding text
	std::string labelInfo = "Label Info"; // default label info
	bool isLandscape = false; // default to portrait
};

class LabelLayoutCalculator
{
public:
	LabelLayoutCalculator(int yBase, int imageHeight, int textHeight) : _yBase(yBase), _imageHeight(imageHeight), _textHeight(textHeight)
	{
	}

	int lineY(int lineIndex) const
	{
		return _yBase + _imageHeight + (_textHeight - 2) * lineIndex;
	}

private:
	int _yBase;
	int _imageHeight;
	int _textHeight;
};