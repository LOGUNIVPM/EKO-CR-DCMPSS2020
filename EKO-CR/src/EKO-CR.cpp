#include "EKO-CR.hpp"

Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelDirac);
	p->addModel(modelEkoPanel);
    p->addModel(modelTimbale);
    p->addModel(modelSnareDrum);
    p->addModel(modelCharleston);
}
