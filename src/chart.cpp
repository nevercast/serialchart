#include "chart.h"
#include "tinyexpr/tinyexpr.h"

Chart::Chart(QWidget *parent) :
    QWidget(parent),config(0)
{
}

void Chart::init(Configuration* config){
    this->config = config;
    this->setFixedSize(config->get("_setup_","width").toInt(),config->get("_setup_","height").toInt());
    this->repaint();

    px = QPixmap(width(),height());
    px.fill(QColor(Qt::transparent));

    prevPoints.clear();
    cursor_pos = 0;

}

void Chart::newPacket(DecoderBase* decoder){
    QList<QVariant> packetValues = decoder->getPacketValues();
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

    QPainter p(&px);
    //clear one vertical line
    p.setPen(Qt::transparent);
    p.setCompositionMode(QPainter::CompositionMode_Clear);
    p.drawLine(cursor_pos,0,cursor_pos,height()-1); //clear vertical line
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for(int i=0; i < MIN(packetValues.length(),config->fields.length()); i++){
        QString sectionName = config->fields[i];

        double vv = packetValues[i].toDouble();
        double min = config->get(sectionName,"min").toDouble();
        double max = config->get(sectionName,"max").toDouble();
        double height = this->height()-1;   //pixel height of the canvas reduce by 1 since in case height is 1 max and min coincide ...


        //by default a value of "min" will map to 0 on chart and a value of "max" to (this->height()-1)
        double pitch = config->get(sectionName,"pitch", QString::number( (max-min) / height) ).toDouble();    //pitch in units per pixel
        // max / -min  = h_origin / (height - h_origin) => h_origin = height * max / (max-min(
        double h_origin = config->get(sectionName,"h_origin", QString::number(  height * max/(max-min))  ).toDouble();
        double y = h_origin - vv / pitch;


        int dash = config->get(sectionName,"dash").toInt();
        if(dash>0 && 1 == (cursor_pos / dash) % 2){
            p.setPen(Qt::transparent);       //dash spacing
        }else{
            p.setPen(QColor(config->get(sectionName,"color")));
        }

        QPoint newPoint(cursor_pos,round(y));
        if(prevPoints.length()-1>=i){
            QPoint prevPoint = prevPoints[i];
            if(prevPoint.x()>newPoint.x()) prevPoint.setX(-1);
            p.drawLine(prevPoint,newPoint);
            prevPoints.replace(i,newPoint);
        }else{
            p.drawPoint(newPoint);
            prevPoints.append(newPoint);
        }

    }
    cursor_pos = (cursor_pos + 1) % width();
    this->repaint();
}



void Chart::paintEvent(QPaintEvent* ev){
    QWidget::paintEvent(ev);
    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing,true);

    if(config){
        //background
        p.setPen(Qt::NoPen);
        p.setBrush(QBrush(QColor(config->get("_setup_","background_color"))));
        p.drawRect(rect());
        p.setBrush(Qt::NoBrush);

        //grid: horizontal lines
        int step = config->get("_setup_","grid_h_step").toInt();
        int orig = config->get("_setup_","grid_h_origin").toInt();
        p.setPen(QColor(config->get("_setup_","grid_h_color")));
        if(step>0){
            for(int i=orig;i>=0;i-=step) p.drawLine(0,i,width()-1,i);
            for(int i=orig;i<height();i+=step) p.drawLine(0,i,width()-1,i);
        }
        //grid: vertical lines
        step = config->get("_setup_","grid_v_step").toInt();
        orig = config->get("_setup_","grid_v_origin").toInt();
        p.setPen(QColor(config->get("_setup_","grid_v_color")));
        if(step>0){
            for(int i=orig;i>=0;i-=step) p.drawLine(i,0,i,height()-1);
            for(int i=orig;i<width();i+=step) p.drawLine(i,0,i,height()-1);
        }
        //axes
        p.setPen(QColor(config->get("_setup_","grid_v_origin_color")));
        p.drawLine(orig,0,orig,height()-1);
        orig = config->get("_setup_","grid_h_origin").toInt();
        p.setPen(QColor(config->get("_setup_","grid_h_origin_color")));
        p.drawLine(0,orig,width()-1,orig);
    }

    p.drawPixmap(0,0,px);

}
