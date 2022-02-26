#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

extern Model* modelDirac;
extern Model* modelEkoPanel;
extern Model* modelTimbale;
extern Model* modelSnareDrum;
extern Model* modelCharleston;

/* ------- GUI ---------- */

// Voice Buttons
struct EkoVoiceButton : app::SvgSwitch {
	EkoVoiceButton() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-voiceOFF.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-voiceON.svg")));
	}
};

// Horz Volume Slider
struct EkoHorzVolumeSlider : app::SvgSlider {
	EkoHorzVolumeSlider() {
		//math::Vec margin = math::Vec(3.5, 3.5);
		horizontal = true;
		maxHandlePos = mm2px(math::Vec(22.078, 0.738).plus(math::Vec(0, 2)));
		minHandlePos = mm2px(math::Vec(0.738, 0.738).plus(math::Vec(0, 2)));
		setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-sliderBG.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-sliderCAP.svg")));

		//background->box.pos = margin;
		//box.size = background->box.size.plus(margin.mult(2));
	}
};

// Step Buttons
struct EkoStepButton : app::SvgSwitch {
	EkoStepButton() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-stepOFF.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-stepON.svg")));
	}
};

// Volume and Speed knobs
struct EkoKnob : SvgKnob {
	EkoKnob() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-knob.svg")));
	}
};
// ABC STUFF


struct ATextLabel : TransparentWidget {
	std::shared_ptr<Font> font;
	NVGcolor txtCol;
	char text[128];
	const int fh = 14;

	ATextLabel(Vec pos) {
		box.pos = pos;
		box.size.y = fh;
		setColor(0x00, 0x00, 0x00, 0xFF);
		setText(" ");
	}

	ATextLabel(Vec pos, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		box.pos = pos;
		box.size.y = fh;
		setColor(r, g, b, a);
		setText(" ");
	}

	void setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		txtCol.r = r;
		txtCol.g = g;
		txtCol.b = b;
		txtCol.a = a;
	}

	void setText(const char * txt) {
		strncpy(text, txt, sizeof(text));
		box.size.x = strlen(text) * 8;
	}

	void drawBG(const DrawArgs &args) {
		Vec c = Vec(box.size.x/2, box.size.y);
		const int whalf = box.size.x/2;

		// Draw rectangle
		nvgFillColor(args.vg, nvgRGBA(0xF0, 0xF0, 0xF0, 0xFF));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, c.x -whalf, c.y +2);
			nvgLineTo(args.vg, c.x +whalf, c.y +2);
			nvgLineTo(args.vg, c.x +whalf, c.y+fh+2);
			nvgLineTo(args.vg, c.x -whalf, c.y+fh+2);
			nvgLineTo(args.vg, c.x -whalf, c.y +2);
			nvgClosePath(args.vg);
		}
		nvgFill(args.vg);
	}

	void drawTxt(const DrawArgs &args, const char * txt) {
        
        font = APP->window->loadFont("res/fonts/DejaVuSans.ttf");
		Vec c = Vec(box.size.x/2, box.size.y);

		nvgFontSize(args.vg, fh);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGBA(txtCol.r, txtCol.g, txtCol.b, txtCol.a));

		nvgText(args.vg, c.x, c.y+fh, txt, NULL);
	}

	void draw(const DrawArgs &args) override {
		TransparentWidget::draw(args);
		drawBG(args);
		drawTxt(args, text);
	}



};

struct ATextHeading : TransparentWidget {
	std::shared_ptr<Font> font;
	NVGcolor txtCol;
	char text[128];
	const int fh = 14;

	ATextHeading(Vec pos) {
		box.pos = pos;
		box.size.y = fh;
		setColor(0xFF, 0xFF, 0xFF, 0xFF);
		setText(" ");
	}

	ATextHeading(Vec pos, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		box.pos = pos;
		box.size.y = fh;
		setColor(r, g, b, a);
		setText(" ");
	}

	void setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		txtCol.r = r;
		txtCol.g = g;
		txtCol.b = b;
		txtCol.a = a;
	}

	void setText(const char * txt) {
		strncpy(text, txt, sizeof(text));
		box.size.x = strlen(text) * 8;
	}

	void draw(const DrawArgs &args) override {
		TransparentWidget::draw(args);
		drawBG(args);
		drawTxt(args, text);
	}

	void drawBG(const DrawArgs &args) {
		Vec c = Vec(box.size.x/2, box.size.y);
		const int whalf = box.size.x/2;

		// Draw rectangle
		nvgFillColor(args.vg, nvgRGBA(0xAA, 0xCC, 0xFF, 0xFF));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, c.x -whalf, c.y +2);
			nvgLineTo(args.vg, c.x +whalf, c.y +2);
			nvgLineTo(args.vg, c.x +whalf, c.y+fh+2);
			nvgLineTo(args.vg, c.x -whalf, c.y+fh+2);
			nvgLineTo(args.vg, c.x -whalf, c.y +2);
			nvgClosePath(args.vg);
		}
		nvgFill(args.vg);
	}

	void drawTxt(const DrawArgs &args, const char * txt) {

        font = APP->window->loadFont("res/fonts/DejaVuSans.ttf");
		Vec c = Vec(box.size.x/2, box.size.y);

		nvgFontSize(args.vg, fh);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGBA(txtCol.r, txtCol.g, txtCol.b, txtCol.a));

		nvgText(args.vg, c.x, c.y+fh, txt, NULL);
	}

};

struct ATitle: TransparentWidget {
	std::shared_ptr<Font> font;
	NVGcolor txtCol;
	char text[128];
	int fh = 20;
	float parentW = 0;

	ATitle(float pW) {
		parentW = pW;
		box.pos = Vec(1 , 1);
		box.size.y = fh;
		setColor(0x55, 0x99, 0xFF, 0xFF);
		setText(" ");
	}

	void setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		txtCol.r = r;
		txtCol.g = g;
		txtCol.b = b;
		txtCol.a = a;
	}

	void setText(const char * txt) {
		strncpy(text, txt, sizeof(text));
		box.size.x = strlen(text) * 10;
	}

	void draw(const DrawArgs &args) override {
		TransparentWidget::draw(args);
		drawTxt(args, text);
	}

	void drawTxt(const DrawArgs &args, const char * txt) {
		float bounds[4];
		Vec c = Vec(box.pos.x, box.pos.y);
        font = APP->window->loadFont("res/fonts/DejaVuSans.ttf");

		nvgFontSize(args.vg, fh);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT);

		// CHECK WHETHER TEXT FITS IN THE MODULE
		nvgTextBounds(args.vg, c.x, c.y, txt, NULL, bounds);
		float xmax = bounds[2];
		if (xmax > parentW) {
			float ratio = parentW / xmax;
			fh = (int)floor(ratio * fh); // reduce fontsize to fit the parent width
		} else {
			c.x += (parentW - xmax)/2; // center text
		}

		nvgFillColor(args.vg, nvgRGBA(txtCol.r, txtCol.g, txtCol.b, txtCol.a));
		nvgText(args.vg, c.x, c.y+fh, txt, NULL);
	}

};

