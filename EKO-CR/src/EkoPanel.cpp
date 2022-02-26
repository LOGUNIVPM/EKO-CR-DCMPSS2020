#include "EKO-CR.hpp"

struct EkoPanel : Module {
	enum ParamIds {
		VOICE_BUTT_1 = 0,
		/* ... */
		VOICE_BUTT_12 = 11,
		STEP_0 = 12,
		STEP_96 = 108,
		VOLUME_1 = 109,
		VOLUME_6 = 115,
		START_STOP,
		MAIN_VOLUME,
		TEMPO_KNOB,
		SEQ_STOP_x5,
		SEQ_STOP_x6,
		SEQ_STOP_x9,
		SEQ_STOP_x10,
		SEQ_STOP_x12,
		SEQ_STOP_x15,
		SEQ_STOP_x16,
		NUM_PARAMS,
	};
	enum InputIds {
		TRIGGER_INPUT,
		NUM_INPUTS,
	};
	enum OutputIds {
		MAIN_OUTPUT,
		NUM_OUTPUTS,
	};

	enum LightsIds {
		LIGHT_TRIGGER,
		NUM_LIGHTS,
	};

	dsp::SchmittTrigger ts;

	EkoPanel() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override;

};

void EkoPanel::process(const ProcessArgs &args) {

	float in = 0.f;

	if (inputs[TRIGGER_INPUT].isConnected()) {
		in = inputs[TRIGGER_INPUT].getVoltage();
	}

	float dirac = ts.process(in);
	lights[LIGHT_TRIGGER].setBrightness(dirac);

	if (outputs[MAIN_OUTPUT].isConnected()) {
		outputs[MAIN_OUTPUT].setVoltage(10.f * dirac);
	}

}

struct EkoPanelWidget : ModuleWidget {

	EkoPanelWidget(EkoPanel* module) {
			int i, j;
			setModule(module);
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EKO-CR-panel.svg")));

			for (i = 0; i < 12; i+=2) {
				addParam(createParamCentered<EkoVoiceButton>(Vec(95, 50+i*23), module, EkoPanel::VOICE_BUTT_1+i));
				addParam(createParamCentered<EkoVoiceButton>(Vec(122, 50+i*23), module, EkoPanel::VOICE_BUTT_1+i+1));
			}

			for (i = 0; i < 6; i++) {
				for (j = 0; j < 16; j++) {
					addParam(createParamCentered<EkoStepButton>(Vec(158+j*23.5, 50+i*46), module, EkoPanel::STEP_0+j+i*16));
				}
			}


			addParam(createParamCentered<EkoKnob>(Vec(60, 355), module, EkoPanel::MAIN_VOLUME));
			addParam(createParamCentered<EkoKnob>(Vec(140, 355), module, EkoPanel::TEMPO_KNOB));
			addParam(createParamCentered<EkoVoiceButton>(Vec(245, 350), module, EkoPanel::START_STOP));

#define SEQ_STOP_BUTT_x 310
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x, 350), module, EkoPanel::SEQ_STOP_x5));
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x+25, 350), module, EkoPanel::SEQ_STOP_x6));
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x+50, 350), module, EkoPanel::SEQ_STOP_x9));
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x+75, 350), module, EkoPanel::SEQ_STOP_x10));
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x+100, 350), module, EkoPanel::SEQ_STOP_x12));
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x+125, 350), module, EkoPanel::SEQ_STOP_x15));
			addParam(createParamCentered<EkoVoiceButton>(Vec(SEQ_STOP_BUTT_x+150, 350), module, EkoPanel::SEQ_STOP_x16));


		}

};

Model * modelEkoPanel = createModel<EkoPanel, EkoPanelWidget>("EkoPanel");
