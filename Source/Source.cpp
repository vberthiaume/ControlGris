#include "Source.h"

Source::Source() {
    m_azimuth = 0.0;
    m_elevation = 0.0;
    m_distance = 1.0;
    colour = Colours::black;
}

Source::~Source() {}

void Source::setId(int id) {
    m_id = id;
}

int Source::getId() {
    return m_id;
}

void Source::setAzimuth(float azimuth) {
    m_azimuth = azimuth;
}

float Source::getAzimuth() {
    return m_azimuth;
}

void Source::setElevation(float elevation) {
    if (elevation < 0.0) {
        m_elevation = 0.0;
    } else if (elevation > 90.0) {
        m_elevation = 90.0;
    } else {
        m_elevation = elevation;
    }
}

float Source::getElevation() {
    return m_elevation;
}

void Source::setDistance(float distance) {
    m_distance = distance;
}

float Source::getDistance() {
    return m_distance;
}

void Source::setColour(Colour col) {
    colour = col;
}

Colour Source::getColour() {
    return colour;
}