//Timbale module

#include "EKO-CR.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using std::cout;
using std::endl;

struct timbale : Module {
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

	dsp::SchmittTrigger edgeDetector; //edge detection with SchmittTrigger

	float* buffer;

	bool reading = false;

	bool inputLow = false;

	float counter = 0;

	int samples;

	int fileFreq;

	timbale();

	void process(const ProcessArgs &args) override;

};

typedef struct header_file
	{
	    char chunk_id[4];
	    int chunk_size;
	    char format[4];
	    char subchunk1_id[4];
	    int subchunk1_size;
	    short int audio_format;
	    short int num_channels;
	    int sample_rate;
	    int byte_rate;
	    short int block_align;
	    short int bits_per_sample;
	    char subchunk2_id[4];
	    int subchunk2_size;
	} header;

	typedef struct header_file* header_p;
timbale::timbale() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	FILE * infile = fopen("./plugins/EKO-CR/res/Timbale.wav", "rb");
	header_p meta = (header_p)malloc(sizeof(header));
	int nb;	// variable storing number of byes returned
	if (infile)
	{
		fread(meta, 1, sizeof(header), infile);
		//cout << " Number of bytes in wave file are " << meta->subchunk2_size << " bytes" << endl;
		//cout << " Number of bits per sample " << meta->bits_per_sample << " bits" << endl;
		short int *buff16 = (short int*) malloc(meta->subchunk2_size);	// because is 16 bit PCM audio
		nb = fread(buff16, 1, meta->subchunk2_size,  infile);
		//cout << " Sampling rate of the input wave file is "<< meta->sample_rate <<" Hz" << endl;
		fileFreq = meta->sample_rate;
		samples = meta->subchunk2_size / (meta->bits_per_sample / 8);
		//Scaling:
		const float MAX = 32767; //float dimension
		buffer = (float*) malloc(sizeof(float) * samples);
		for (int i = 0; i < samples; i++) {
			buffer[i] = buff16[i] * 10 / MAX; //vcvRack works with [-10, 10]V
		}
	}
}

void timbale::process(const ProcessArgs &args) {
	float rate = args.sampleRate;
	float passo = fileFreq / rate;
	bool input = edgeDetector.process(inputs[TRIGGER_INPUT].getVoltage());
	if (input && !reading) {
		reading = true;
		counter = 0;
		inputLow = false;
	}
	else if (!input) {
		inputLow = true;
	}
	else if (input && reading && inputLow) {
		counter = 0;
	}

	if (reading) {
		outputs[MAIN_OUTPUT].setVoltage(math::interpolateLinear(buffer, counter));
		counter += passo;
		if (counter >= samples) reading = false;
	}
	else {
		outputs[MAIN_OUTPUT].setVoltage(0.0f);
	}

}


struct timbaleWidget : ModuleWidget {

	timbaleWidget(timbale* module) {

			setModule(module);
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
			box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
//why doesn't work?

			{
				ATitle * title = new ATitle(box.size.x);
				title->setText("Timbale");
				addChild(title);
			}

			{
				ATextLabel * title = new ATextLabel(Vec(13, 250));
				title->setText("IN");
				addChild(title);
			}

			{
				ATextLabel * title = new ATextLabel(Vec(55, 250));
				title->setText("OUT");
				addChild(title);
			}

			addInput(createInput<PJ301MPort>(Vec(10, 200), module, timbale::TRIGGER_INPUT));

			addOutput(createOutput<PJ301MPort>(Vec(55, 200), module, timbale::MAIN_OUTPUT));

			//addParam(createParam<RoundBlackKnob>(Vec(30, 110), module, timbale::PARAM));

			//addChild(createLight<SmallLight<GreenLight>>(Vec(65, 240), module, timbale::LIGHT_TRIGGER));

	}
};

Model * modelTimbale = createModel<timbale, timbaleWidget>("timbale");




