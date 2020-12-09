#include "EKO-CR.hpp"

struct dirac : Module {
	enum ParamIds {
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

	dirac() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override;

};

void dirac::process(const ProcessArgs &args) {

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

struct diracWidget : ModuleWidget {

	diracWidget(dirac* module) {

			setModule(module);
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
			box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.077, 28.048)), module, dirac::TRIGGER_INPUT));

			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.675, 50.146)), module, dirac::MAIN_OUTPUT));

			addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(27.781, 50.448)), module, dirac::LIGHT_TRIGGER));

		}

};

Model * modelDirac = createModel<dirac, diracWidget>("dirac");
