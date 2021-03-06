/*******************************************************************************
 * Copyright (c) 2020, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef OPENER_CIPSECURITY_H
#define OPENER_CIPSECURITY_H

#include "typedefs.h"
#include "ciptypes.h"

/** @brief CIP Security object class code */
static const CipUint kCipSecurityObjectClassCode = 0x5DU;

/** @brief CIP Security object Begin_Config service code */
static const CipUint kCipSecurityObjectBeginConfigServiceCode = 0x4BU;
/** @brief CIP Security object Kick_Timer service code */
static const CipUint kCipSecurityObjectKickTimerServiceCode = 0x4CU;
/** @brief CIP Security object End_Config service code */
static const CipUint kCipSecurityObjectEndConfigServiceCode = 0x4DU;
/** @brief CIP Security object Object_Cleanup service code */
static const CipUint kCipSecurityObjectCleanupServiceCode = 0x4EU;
/** @brief CIP Security object reset service code */
static const CipUint kCipSecurityObjectResetServiceCode = 0x05U;

/* *****************************************************************************
 * Type declarations
 */
/** @brief Valid values for CIP Security Object State (attribute #1)
 *  The State attribute reports the CIP Security Object’s current state.
 *  see Volume 8, Chapter 5-3.4.1
 */
typedef enum cip_security_object_state_values {
  /** Factory Default Configuration */
  kCipSecurityObjectStateFactoryDefaultConfiguration = 0,
  /** ConfigurationIn Progress */
  kCipSecurityObjectStateConfigurationInProgress,
  /** Configured */
  kCipSecurityObjectStateConfigured,
  /** Incomplete Configuration */
  kCipSecurityObjectStateIncompleteConfiguration
} CipSecurityObjectStateValue;

/** @brief Valid values for CIP Security Object Security Profiles (attribute #2)
 *  The Security Profiles attribute represents the Security Profiles
 *  that the device implements.
 *  see Volume 8, Chapter 5-3.4.2
 */
typedef enum cip_security_object_security_profiles_values {
  /** The device supports the EtherNet/IP Integrity Profile (Obsoleted) */
  kCipSecurityObjectEtherNetIpIntegrityProfile = 0,
  /** The device supports the EtherNet/IP Confidentiality Profile */
  kCipSecurityObjectEtherNetIpConfidentialityProfile,
  /** The device supports the CIP Authorization Profile */
  kCipSecurityObjectCipAuthorizationProfile,
} CipSecurityObjectSecurityProfileValue;

/** @brief Type declaration for the CIP Security object
 *
 * This is the type declaration for the CIP Security object.
 */
typedef struct {
  CipUsint state; /** Attribute #1 */
  CipWord security_profile; /** Attribute #2 */
} CipSecurityObject;


/* ********************************************************************
 * global public variables
 */
extern CipSecurityObject g_security;  /**< declaration of CIP Security object instance 1 data */

/* ********************************************************************
 * public functions
 */
/** @brief Initializing the data structures of the CIP Security object
 *
 * @return kEipStatusOk on success, otherwise kEipStatusError
 */
EipStatus CipSecurityInit(void);

#endif  // OPENER_CIPSECURITY_H