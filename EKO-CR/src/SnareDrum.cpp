#include "EKO-CR.hpp"
#include <iostream>
#include <math.h>
#include <malloc.h>
#include "stdint.h"
#include "logger.hpp"

typedef struct
{
    char chunkID[4];
    int32_t chunkSize;
    char format[4];
    char subchunk1ID[4];
    int32_t subchunk1Size;
    int16_t audioFormat;
    int16_t numChannels;
    int32_t sampleRate;
    int32_t byteRate;
    int16_t blockAlign;
    int16_t bitPerSample;
    char subchunk2ID[4];
    int32_t subchunk2Size;
} headerWAV;

int8_t *data;
uint32_t length_data;
uint32_t data_pointer;
int16_t currentSample;
bool playing_now = 0;
float SchmittTrigger_precOut = 0;
headerWAV myHeader;

struct SnareDrum : Module
{
    enum ParamIds
    {
        VOL_PARAM,
        NUM_PARAMS,
    };
    enum InputIds
    {
        TRIGGER_INPUT,
        NUM_INPUTS,
    };
    enum OutputIds
    {
        MAIN_OUTPUT,
        NUM_OUTPUTS,
    };

    enum LightsIds
    {
        PLAYING_LIGHT,
        NUM_LIGHTS,
    };

    dsp::SchmittTrigger ts;

    int Lagrange_interpolation_1_order(float D, int x0, int x1)
    {

        float l0 = 1 - D;
        float l1 = D;

        int out = l0 * (float)x0 + l1 * (float)x1;

        return out;
    }


    SnareDrum()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(VOL_PARAM, 0.0, 10.0, 5.0);
        //WARN("asdasdasd");

        FILE *myWAV;
        myWAV = fopen("./plugins/EKO-CR/res/Snare.wav", "rb"); //apro file binario (wav) in lettura

        if (myWAV == NULL)
        { //se non posso aprire il file stampo messaggio di errore e esco
            FATAL("Impossibile aprire il file wav.");
            exit(1);
        }



        fread(&myHeader, sizeof(char), 4, myWAV); //leggo i primi 4 byte del file per controllare se effettivamente è un wav
        bool flag = true;                         //inizializzo il flag a true, se rimane tale dopo il for allora il file è un wav

        for (int i = 0; i < 4; i++)
        {
            if (myHeader.chunkID[i] != "RIFF"[i])
            {                 //se uno dei caratteri non coincide con quello dell'intestazione
                flag = false; //metto il flag a false e interrompo il ciclo iterativo in modo prematuro
                break;
            }
        }

        if (flag == false)
        {
            FATAL("Il file non è .wav.");
            exit(1);
        }

        fseek(myWAV, 0, SEEK_SET);                                  //riporto il puntatore interno del file all'inizio
        fread(&myHeader, sizeof(myHeader), 1, myWAV);               //leggo tutta l'intestazione del file e la metto dentro alla struttura myHeader
        length_data = myHeader.subchunk2Size;                       //numero totale di byte (campioni) dopo l'intestazione
        data = (int8_t *)malloc(length_data);                       //allocazione dinamica dell'array di byte dove mettere i campioni
        fread(data, sizeof(int8_t), myHeader.subchunk2Size, myWAV); //leggo i dati e li salvo byte per byte dentro data

        data_pointer = 0; //indice della locazione corrente nell'array data inizialmente messo a 0

        INFO("fine vecchio costruttore");

        SnareDrum::onSampleRateChange();

        INFO("fine onsampleratechange nel costruttore");


    }

    void process(const ProcessArgs &args) override;
    void onSampleRateChange() override;
};


void SnareDrum::onSampleRateChange(){

    int RackSampleRate = (int)(APP->engine->getSampleRate()); //leggo la frequenza di campionamento usata da Rack

    if (myHeader.sampleRate != RackSampleRate)
    { //se la frequenza di campionamento del wav è diversa da quella di Rack
        int16_t *p_data_sample = (int16_t *)data;
        double frequency_ratio = (double)myHeader.sampleRate / (double)RackSampleRate;

        INFO("length_data:%d frequency_ratio:%f", length_data, frequency_ratio);

        int k = 0;                                           //indice della locazione nel futuro array di campioni con la nuova fs (quella di rack)
        double D = 0;                                        //fattore D dell'interpolazione di lagrange (valore iniziale)
        int D_int = 0;                                       //parte intera del fattore D (valore iniziale)
        float D_dec = 0;                                     //parte decimale del fattore D (valore iniziale)
        int new_length_data = length_data / frequency_ratio; //lunghezza del nuovo array di campioni (alla nuova fs)

        new_length_data = new_length_data % 2 ? new_length_data + 1 : new_length_data;
        int8_t *new_data = (int8_t *)malloc(new_length_data); //alloco la memoria per il nuovo array di campioni

        INFO("new_length_data:%d", new_length_data);

        int16_t *p_new_data_sample = (int16_t *)new_data;
        while (D < ((float)length_data / 2)){ //finché D non supera la lunghezza del vecchio array

            D_int = floor(D);
            D_dec = D - (float)D_int;
            //INFO("\n\r%d \n\r\t%d %d\n\r\t%f %d %f",k,*(int16_t*)(data+D_int),*(int16_t*)(data+D_int+1),D,D_int,D_dec);

            *(p_new_data_sample + k) = Lagrange_interpolation_1_order(D_dec, *(p_data_sample + D_int), *(p_data_sample + D_int + 1));
            //INFO("new_data n.%d:%d",k,* ((int16_t*)new_data+k));

            D = D + frequency_ratio;
            k++;
        }

        free(data);
        length_data = new_length_data;
        data = new_data;
        myHeader.sampleRate = RackSampleRate;



        INFO("fine onsampleratechange");
    }


}

void SnareDrum::process(const ProcessArgs &args)
{
    float TriggerSchmitt_in = 0.f; //ingresso del trigger di Schmitt inizialmente messo a 0 (default)

    if (inputs[TRIGGER_INPUT].isConnected())
    { //se l'ingresso è connesso ne leggo il valore, altrimenti rimane lo 0 di default
        TriggerSchmitt_in = inputs[TRIGGER_INPUT].getVoltage();
    }

    float TriggerSchmitt_out = ts.process(TriggerSchmitt_in);                 //valuto l'uscita del trigger di Schmitt
    float TriggerSchmitt_event = TriggerSchmitt_out - SchmittTrigger_precOut; //controllo se l'uscita precedente differisce da quella attuale
    SchmittTrigger_precOut = TriggerSchmitt_out;                              //salvo l'uscita attuale come stato precedente per la prossima volta che userò la funzione

    if (TriggerSchmitt_event == 1)
    {                       //c'è stato un fronte di salita: uscita attuale 1 e uscita precedente 0
        data_pointer = 0;   //setto l'indice della locazione corrente nell'array dei campioni a 0 (inizio)
        playing_now = true; //setto "playing_now" a true per indicare che è iniziata la riproduzione dei campioni
        lights[PLAYING_LIGHT].value = 1;
    }

    if (playing_now == true)
    { //se è in corso la riproduzione dei campioni
        //WARN("playing_now");
        if (data_pointer < length_data - 1)
        {                                                                                                    // se l'indice "data_pointer" fa riferimento ad un dato esistente(precedente all'ultimo)
            currentSample = ((int16_t)data[data_pointer + 1] << 8) | ((int16_t)data[data_pointer] & 0x00ff); //mi salvo il campione corrente (2 byte)
            //currentSample = *((int16_t*)(data+data_pointer));
            data_pointer = data_pointer + 2; //block_align = 2
        }
        else
        {
            currentSample = 0;   //altrimenti pongo come campione corrente 0
            playing_now = false; //e setto "playing_now" a false per indicare che la riproduzione dei campioni è terminata
            lights[PLAYING_LIGHT].value = 0;
            data_pointer = 0;
        }
    }

    if (outputs[MAIN_OUTPUT].isConnected())
    { //se l'uscita è connessa, setto il suo valore in base a quello del campione corrente
        float volume = params[VOL_PARAM].getValue();
        outputs[MAIN_OUTPUT].setVoltage(currentSample / 32768.f * volume); // proporzione: 5V (tensione max) corrisponde a 32768 (valore max del campione)
    }
}

struct SnareDrumWidget : ModuleWidget
{

    SnareDrumWidget(SnareDrum *module)
    {

        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
        box.size = Vec(12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55, 30)), module, SnareDrum::TRIGGER_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55, 50)), module, SnareDrum::MAIN_OUTPUT));

        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(55, 70)), module, SnareDrum::PLAYING_LIGHT));

        addParam(createParam<RoundBlackKnob>(mm2px(Vec(50, 85)), module, SnareDrum::VOL_PARAM));
    }
};

Model *modelSnareDrum = createModel<SnareDrum, SnareDrumWidget>("SnareDrum");
