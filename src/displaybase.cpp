#include "displaybase.h"
#include "tinyexpr/tinyexpr.h"

DisplayBase::DisplayBase(QObject *parent,Configuration* config) :
    QObject(parent)
{
    this->config = config;

}


DisplayBase* createDisplay(QObject *parent,Configuration *config){
    return new DisplayBase(parent,config);
}


void DisplayBase::newPacket(DecoderBase* decoder){
    QByteArray displayResult;
    QString display = config->get("_setup_","display","raw");

    QList<QVariant> packetValues = decoder->getPacketValues();
    QList<QByteArray> packetParts = decoder->getPacketParts();
    int ctValues = packetValues.length();
    double values[ctValues];
    te_variable vars[ctValues];
    QByteArray expressions[ctValues];
    QByteArray variableNames[ctValues];

    int fieldCount = config->fields.length();
    for (int j=0; j < ctValues; j++) {
        values[j] = packetValues[j].toDouble();
        vars[j].address = &values[j];
        if (j < fieldCount) {
            variableNames[j] = config->fields[j].toLatin1();
        } else {
            expressions[j] = variableNames[j] = QString("Field%1").arg(QString::number(j+1)).toLatin1();
        }
        if (j < fieldCount) {
            expressions[j] = config->get(config->fields[j], "expr", QString("Field%1").arg(QString::number(j+1))).toLatin1();
        }
        variableNames[j] = variableNames[j].toLower();
        expressions[j] = expressions[j].toLower();
        vars[j].name = variableNames[j].data();
        vars[j].type = 0;
        vars[j].context = 0;
    }

    // Evaluate expressions
    for (int j=0; j < MIN(config->fields.length(), ctValues); j++) {
        int err;
        te_expr* expression = te_compile(expressions[j].data(), vars, ctValues, &err);
        if (expression == 0) {
            // Error occurred
        } else {
            double result = te_eval(expression);
            packetValues[j] = result;
            te_free(expression);
        }
    }



    // Set display output
    if(display == "list"){
        QByteArray display_sep =  QByteArray::fromPercentEncoding(
            config->get("_setup_","display_sep",",").toLatin1()
        );
        int display_skip_transparent = config->get("_setup_","display_skip_transparent","0").toInt();
        QList<QVariant> packetValues = decoder->getPacketValues();
        QList<QByteArray> packetParts = decoder->getPacketParts();
        bool first = true;
        for(int i=0; i < MIN(config->fields.length(),packetValues.length()); i++){
            QString field = config->fields[i];
            QByteArray format =  config->get(field,"format","%f").toLatin1();
            QString expr = config->get(field, "expr", "");
            int precision = config->get(field,"precision","6").toInt();

            if(!display_skip_transparent || config->get(field,"color") != "transparent"){
                if(!first) displayResult += QString(display_sep);
                QByteArray v = format;


                v.replace(QByteArray("%g"),QByteArray::number(packetValues[i].toDouble(),'g',precision));
                v.replace(QByteArray("%f"),QByteArray::number(packetValues[i].toDouble(),'f',precision));
                v.replace(QByteArray("%d"),QByteArray::number(packetValues[i].toInt(),'f',0));
                v.replace(QByteArray("%n"),field.toLatin1());
                v.replace(QByteArray("%s"),packetParts[i]);
                v.replace(QByteArray("%x"),packetParts[i].toHex());


                displayResult += v;
                first = false;
            }
        }
        emit newDisplay(displayResult);
    }else if(display == "hex"){
        emit newDisplay(decoder->getPacketBytes().toHex());
    }else{
        emit newDisplay(decoder->getPacketBytes());
    }

}
