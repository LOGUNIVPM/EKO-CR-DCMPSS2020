#include "EKO-CR.hpp"

Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelDirac);

}
