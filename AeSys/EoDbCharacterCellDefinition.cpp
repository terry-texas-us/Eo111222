#include "Stdafx.h"

#include "EoDbCharacterCellDefinition.h"

EoDbCharacterCellDefinition::EoDbCharacterCellDefinition(double rotationAngle, double slantAngle,
                                                         double expansionFactor, double height)
    : m_height{height}, m_expansionFactor{expansionFactor}, m_rotationAngle{rotationAngle}, m_slantAngle{slantAngle} {}
