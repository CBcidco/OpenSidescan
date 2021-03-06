#ifndef DETECTIONWINDOW_H
#define DETECTIONWINDOW_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QIntValidator>

#include "sidescanfile.h"
#include "sidescanimage.h"
#include "opencvhelper.h"

class DetectionWindow : public QDialog
{
    Q_OBJECT

public:
    DetectionWindow(std::vector<SidescanFile *> & files,
                    int & fastThresholdValue,
                    int & fastTypeValue,
                    bool & fastNonMaxSuppressionValue,
                    int & dbscanEpsilonValue,
                    int & dbscanMinPointsValue,
                    int & mserDeltaValue,
                    int & mserMinimumAreaValue,
                    int & mserMaximumAreaValue,
                    bool & showFeatureMarkersValue,
                    bool & mergeOverlappingBoundingBoxesValue,
                    QWidget * parent=0
    );


    std::vector<SidescanFile *>  & getFiles() { return files; }

    int getFastThresholdValue() const { return fastThresholdValue; }
    int getFastTypeValue() const { return fastTypeValue; }
    bool getFastNonMaxSuppressionValue() const { return fastNonMaxSuppressionValue; }
    int getDbscanEpsilonValue() const { return dbscanEpsilonValue; }
    int getDbscanMinPointsValue() const { return dbscanMinPointsValue; }
    int getMserDeltaValue() const { return mserDeltaValue; }
    int getMserMinimumAreaValue() const { return mserMinimumAreaValue; }
    int getMserMaximumAreaValue() const { return mserMaximumAreaValue; }
    bool getShowFeatureMarkersValue() const { return showFeatureMarkersValue; }
    bool getMergeOverlappingBoundingBoxesValue() const { return mergeOverlappingBoundingBoxesValue; }






private slots:
    void ok();
    void cancel();

private:
    std::vector<SidescanFile *>  & files;

    std::vector<GeoreferencedObject*> & objects;

    void createFastParameterBox(QFormLayout * advancedParametersLayout);
    void createMserParameterBox(QFormLayout * advancedParametersLayout);
    void createDbscanParameterBox(QFormLayout * advancedParametersLayout);
    QGroupBox * createDisplayParameterBox();

    QDialogButtonBox * buttonBox;

    //display parameters
    QGroupBox * displayParameters;

    QCheckBox * showFeatureMarkers;
    QCheckBox * mergeBoundingBoxes;

    //FAST parameters
    QGroupBox * advancedParameters;

    QLineEdit * fastThreshold;
    QComboBox * fastType;
    QCheckBox * fastNonMaxSuppression;

    //MSER parameters


    QLineEdit * mserDelta;
    QLineEdit * mserMinimumArea;
    QLineEdit * mserMaximumArea;


    //DBSCAN parameters;


    QLineEdit * dbscanEpsilon;
    QLineEdit * dbscanMinimumPointCount;



    //Values
    int & fastThresholdValue;
    int & fastTypeValue;
    bool & fastNonMaxSuppressionValue;
    int & dbscanEpsilonValue;
    int & dbscanMinPointsValue;
    int & mserDeltaValue;
    int & mserMinimumAreaValue;
    int & mserMaximumAreaValue;
    bool & showFeatureMarkersValue;
    bool & mergeOverlappingBoundingBoxesValue;

    void initUI();

};

#endif // DETECTIONWINDOW_H
