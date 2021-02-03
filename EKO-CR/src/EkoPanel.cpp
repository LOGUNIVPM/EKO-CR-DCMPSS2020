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
				addParam(createParamCentered<EkoVoiceButton>(Vec(80, 50+i*23), module, EkoPanel::VOICE_BUTT_1+i));
				addParam(createParamCentered<EkoVoiceButton>(Vec(100, 50+i*23), module, EkoPanel::VOICE_BUTT_1+i+1));
			}

			for (i = 0; i < 6; i++) {
				for (j = 0; j < 16; j++) {
					addParam(createParamCentered<EkoStepButton>(Vec(158+j*23.5, 50+i*46), module, EkoPanel::STEP_0+j+i*16));
				}
			}

//			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.077, 28.048)), module, EkoPanel::TRIGGER_INPUT));
//
//			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.675, 50.146)), module, EkoPanel::MAIN_OUTPUT));
//
//			addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(27.781, 50.448)), module, EkoPanel::LIGHT_TRIGGER));

		}

};

Model * modelEkoPanel = createModel<EkoPanel, EkoPanelWidget>("EkoPanel");
