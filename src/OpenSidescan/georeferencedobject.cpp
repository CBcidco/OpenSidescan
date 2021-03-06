#include "georeferencedobject.h"

#include "../thirdParty/MBES-lib/src/math/Distance.hpp"

#include "../thirdParty/MBES-lib/src/utils/Exception.hpp"
#include "../thirdParty/MBES-lib/src/math/CoordinateTransform.hpp"
#include "../thirdParty/MBES-lib/src/sidescan/SideScanGeoreferencing.hpp"
#include "../thirdParty/MBES-lib/src/math/Distance.hpp"

#include <algorithm>

GeoreferencedObject::GeoreferencedObject(SidescanFile & file,SidescanImage & image,int x,int y,int pixelWidth,int pixelHeight,std::string name, std::string description) :
    file(file) ,
    image(image),
    startPing(*image.getPings().at( y)       ),
    endPing(*image.getPings().at( std::min((int) y+pixelHeight,(int) image.getPings().size())) ),
    x(x),
    y(y),
    pixelWidth(pixelWidth),
    pixelHeight(std::min((int)pixelHeight,(int)image.getPings().size())),
    name(name),
    description(description)
{
    yCenter = y + (pixelHeight/2);
    xCenter = x + (pixelWidth/2);

    computeDimensions();
    computePosition();
}

GeoreferencedObject::~GeoreferencedObject(){
    if ( position != nullptr )
        delete position;

}

void GeoreferencedObject::computeDimensions(){
    //FIXME: not as accurate as we could go, ok for ballpark

    if(yCenter < image.getPings().size()){
        width = pixelWidth * image.getPings().at(yCenter)->getDistancePerSample();
    }
    else{
        width = 0;
    }

    if( y < image.getPings().size() && y+pixelHeight < image.getPings().size() ){
        Position * pos1 = image.getPings()[y]->getPosition();

        if ( pos1 == nullptr )
            throw new Exception( "GeoreferencedObject::computeDimensions(): pos1 is nullptr (y: " + std::to_string( y ) + ")" );

        Position * pos2  = image.getPings()[y+pixelHeight]->getPosition();

        if ( pos2 == nullptr )
            throw new Exception( "GeoreferencedObject::computeDimensions(): pos2 is nullptr (y: " + std::to_string( y )
                                    + ", pixelHeight: " + std::to_string( pixelHeight) + ")" );

        height = Distance::haversine(pos1->getLongitude(),pos1->getLatitude(),pos2->getLongitude(),pos2->getLatitude());
    }
    else{
        height = 0;
    }

}

void GeoreferencedObject::computePosition(){

    if(yCenter < image.getPings().size()){
        SidescanPing * pingCenter = image.getPings()[yCenter];





        Eigen::Vector3d shipPositionEcef;
        CoordinateTransform::getPositionECEF(shipPositionEcef, *pingCenter->getPosition());

        Eigen::Vector3d tangentUnitVector;
        bool tangentFound = false;
        int offset = 0;

        while(!tangentFound) {
            ++offset;
            if(yCenter + offset < image.getPings().size()) {
                SidescanPing * pingAfter = image.getPings()[yCenter+offset];

                if(pingAfter->getTimestamp() < pingCenter->getTimestamp()) {
                    throw new Exception("GeoreferencedObject::computePosition(): pingAfter [yCenter+1] has lower Timestamp than pingCenter");
                }

                if(Distance::haversine(
                            pingAfter->getPosition()->getLongitude(),
                            pingAfter->getPosition()->getLatitude(),
                            pingCenter->getPosition()->getLongitude(),
                            pingCenter->getPosition()->getLatitude()) > 1.0)
                {
                    Eigen::Vector3d positionAfterECEF;
                    CoordinateTransform::getPositionECEF(positionAfterECEF, *pingAfter->getPosition());
                    tangentUnitVector = (positionAfterECEF - shipPositionEcef).normalized();
                    tangentFound = true;
                }
            } else if(yCenter - offset > 0) {
                SidescanPing * pingBefore = image.getPings()[yCenter - offset];

                if(pingBefore->getTimestamp() > pingCenter->getTimestamp()) {
                    throw new Exception("GeoreferencedObject::computePosition(): pingBefore [yCenter-1] has greater Timestamp than pingCenter");
                }

                if(Distance::haversine(
                            pingBefore->getPosition()->getLongitude(),
                            pingBefore->getPosition()->getLatitude(),
                            pingCenter->getPosition()->getLongitude(),
                            pingCenter->getPosition()->getLatitude()) > 1.0)
                {
                    Eigen::Vector3d positionBeforeECEF;
                    CoordinateTransform::getPositionECEF(positionBeforeECEF, *pingBefore->getPosition());
                    tangentUnitVector = (shipPositionEcef - positionBeforeECEF).normalized();
                    tangentFound = true;
                }
            } else {
                throw new Exception("GeoreferencedObject::computePosition(): Could not find another position to compute tangent vector to ship trajectory.");
            }
        }

        Eigen::Vector3d normalUnitVector = shipPositionEcef.normalized();

        Eigen::Vector3d starboardUnitVector = tangentUnitVector.cross(normalUnitVector);
        Eigen::Vector3d portUnitVector = -starboardUnitVector;

        double delta = pingCenter->getDistancePerSample();
        double distanceToObject = delta*yCenter;

        Eigen::Vector3d sideScanDistanceECEF;

        if(image.isStarboard()) {
            sideScanDistanceECEF = starboardUnitVector*distanceToObject;
        } else if(image.isPort()) {
            sideScanDistanceECEF = portUnitVector*distanceToObject;
        } else {
            position = new Position(
                        pingCenter->getPosition()->getTimestamp(),
                        pingCenter->getPosition()->getLatitude(),
                        pingCenter->getPosition()->getLongitude(),
                        pingCenter->getPosition()->getEllipsoidalHeight());
            return; // this image is neither port nor starboard. For now, use ship position
        }

        //Eigen::Matrix3d C_bI_n;
        //CoordinateTransform::getDCM(C_bI_n, *pingCenter->getAttitude());

        //Eigen::Matrix3d C_n_Ecef;
        //CoordinateTransform::ned2ecef(C_n_Ecef,*pingCenter->getPosition());

        // TODO: get this lever arm from platform metadata
        // Eigen::Vector3d antenna2TowPointEcef = C_n_Ecef*C_bI_n*platformMetadata.getAntenna2TowPointLeverArm();
        Eigen::Vector3d antenna2TowPointEcef(0,0,0); // Zero vector for now


        // TODO: get this layback from xtf file
        // -tangent vector since tow fish is assumed to be directly behind ship
        // -normal vector since sensorDepth is positive down (according to XTF doc)
        Eigen::Vector3d backEcef = pingCenter->getLayback()*(-tangentUnitVector);
        Eigen::Vector3d downEcef = pingCenter->getSensorDepth()*(-normalUnitVector);
        Eigen::Vector3d laybackEcef = backEcef + downEcef;

        position = new Position(pingCenter->getTimestamp(), 0.0, 0.0, 0.0);
        //set position of this georeferenced object
        SideScanGeoreferencing::georeferenceSideScanEcef(shipPositionEcef, antenna2TowPointEcef, laybackEcef, sideScanDistanceECEF, *position);
    }
    else{
        position = NULL;
    }

    if ( position == nullptr )
        throw new Exception( "GeoreferencedObject::computePosition(): position is nullptr" );
}
