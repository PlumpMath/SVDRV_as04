TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -isystem /usr/include/panda3d/
QMAKE_CXXFLAGS += -isystem /usr/include/python2.7/
QMAKE_CXXFLAGS += -isystem /usr/include/eigen3/
QMAKE_CXXFLAGS += -isystem /usr/local/bullet/
INCLUDEPATH += /usr/include/panda3d/
INCLUDEPATH += /usr/include/python2.7/
INCLUDEPATH += /usr/include/eigen3/
INCLUDEPATH += /usr/include/bullet/

LIBS += -L/usr/lib/panda3d
LIBS += -lp3framework
LIBS += -lpanda
LIBS += -lpandafx
LIBS += -lpandaexpress
LIBS += -lp3dtoolconfig
LIBS += -lp3dtool
LIBS += -lp3pystub
LIBS += -lp3direct
LIBS += -pthread

LIBS += -lpandabullet
LIBS += -lBulletCollision
LIBS += -lBulletDynamics
LIBS += -lLinearMath

OTHER_FILES += \
    texture/floor.jpg \
    texture/roof.jpg \
    texture/wall.jpg \
    texture/cursor.png \
    sounds/main_bg.ogg \
    sounds/shot_sound.flac \
    sounds/shot_sound.wav \
    models/bigtable/maps/TextureMap.tif \
    models/bigtable/BigTable.egg \
    models/buildingblock/maps/block2.tif \
    models/buildingblock/BuildingBlock.egg \
    models/football/maps/footballtexture2.tif \
    models/football/football.egg
