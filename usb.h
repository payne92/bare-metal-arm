

// USB standard request codes
#define mGET_STATUS           0
#define mCLR_FEATURE          1
#define mSET_FEATURE          3
#define mSET_ADDRESS          5
#define mGET_DESC             6
#define mSET_DESC             7
#define mGET_CONFIG           8
#define mSET_CONFIG           9
#define mGET_INTF             10
#define mSET_INTF             11
#define mSYNC_FRAME           12

// USB descriptor types
#define mDEVICE             1
#define mCONFIGURATION      2
#define mSTRING             3
#define mINTERFACE          4
#define mENDPOINT           5
#define mDEVICE_QUALIFIER   6
#define mOTHER_SPEED_CONFIGURATION  7
#define mINTERFACE_POWER        8

// USB setup packet structure (assumes compiler is little endian!)
typedef struct {
    uint8_t     bmRequestType;
    uint8_t     bRequest;
    uint16_t    wValue;
    uint16_t    wIndex;
    uint16_t    wLength;
} USB_SETUP;

// Device descriptor
typedef struct {
    uint8_t     bLength;
    uint8_t     bDscType;
    uint16_t    bcdUSB;
    uint8_t     bDevCls;
    uint8_t     bDevSubCls;    
    uint8_t     bDevProtocol;
    uint8_t     bMaxPktSize0;  
    uint16_t    idVendor;      
    uint16_t    idProduct;
    uint16_t    bcdDevice;
    uint8_t     iMFR;          
    uint8_t     iProduct;
    uint8_t     iSerialNum;    
    uint8_t     bNumCfg;
} __attribute__((packed)) USB_DEV_DSC;

// Configuration descriptor
typedef struct {
    uint8_t     bLength;
    uint8_t     bDscType;
    uint16_t    wTotalLength;
    uint8_t     bNumIntf;
    uint8_t     bCfgValue;
    uint8_t     iCfg;
    uint8_t     bmAttributes;
    uint8_t     bMaxPower;
} __attribute__((packed)) USB_CFG_DSC;

// Interface descriptor
typedef struct {
    uint8_t     bLength;
    uint8_t     bDscType;
    uint8_t     bIntfNum;
    uint8_t     bAltSetting;
    uint8_t     bNumEPs;
    uint8_t     bIntfCls;
    uint8_t     bIntfSubCls;
    uint8_t     bIntfProtocol;
    uint8_t     iIntf;
} __attribute__((packed)) USB_INTF_DSC;

// Endpoint descriptor
typedef struct {
    uint8_t     bLength;
    uint8_t     bDscType;
    uint8_t     bEPAdr;
    uint8_t     bmAttributes;
    uint16_t    wMaxPktSize;
    uint8_t     bInterval;
} __attribute__((packed)) USB_EP_DSC;

// --------------------------------------------------------------------------------------

#define CDC_COMM_INTF_ID        0x00
#define CDC_COMM_UEP            UEP2
#define CDC_INT_BD_IN           ep2Bi
#define CDC_INT_EP_SIZE         8

#define CDC_DATA_INTF_ID        0x01
#define CDC_DATA_UEP            UEP3
#define CDC_BULK_BD_OUT         ep3Bo
#define CDC_BULK_OUT_EP_SIZE    8
#define CDC_BULK_BD_IN          ep3Bi
#define CDC_BULK_IN_EP_SIZE     8

/* Class-Specific Requests */
#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23

/* Notifications *
 * Note: Notifications are polled over
 * Communication Interface (Interrupt Endpoint)
 */
#define NETWORK_CONNECTION          0x00
#define RESPONSE_AVAILABLE          0x01
#define SERIAL_STATE                0x20

/* Device Class Code */
#define CDC_DEVICE                  0x02

/* Communication Interface Class Code */
#define COMM_INTF                   0x02

/* Communication Interface Class SubClass Codes */
#define ABSTRACT_CONTROL_MODEL      0x02

/* Communication Interface Class Control Protocol Codes */
#define V25TER                      0x01    // Common AT commands ("Hayes(TM)")


/* Data Interface Class Codes */
#define DATA_INTF                   0x0A

/* Data Interface Class Protocol Codes */
#define NO_PROTOCOL                 0x00    // No class specific protocol required


/* Communication Feature Selector Codes */
#define ABSTRACT_STATE              0x01
#define COUNTRY_SETTING             0x02

/* Functional Descriptors */
/* Type Values for the bDscType Field */
#define CS_INTERFACE                0x24
#define CS_ENDPOINT                 0x25

/* bDscSubType in Functional Descriptors */
#define DSC_FN_HEADER               0x00
#define DSC_FN_CALL_MGT             0x01
#define DSC_FN_ACM                  0x02    // ACM - Abstract Control Management
#define DSC_FN_DLM                  0x03    // DLM - Direct Line Managment
#define DSC_FN_TELEPHONE_RINGER     0x04
#define DSC_FN_RPT_CAPABILITIES     0x05
#define DSC_FN_UNION                0x06
#define DSC_FN_COUNTRY_SELECTION    0x07
#define DSC_FN_TEL_OP_MODES         0x08
#define DSC_FN_USB_TERMINAL         0x09
/* more.... see Table 25 in USB CDC Specification 1.1 */

/* CDC Bulk IN transfer states */
#define CDC_TX_READY                0
#define CDC_TX_BUSY                 1
#define CDC_TX_BUSY_ZLP             2       // ZLP: Zero Length Packet
#define CDC_TX_COMPLETING           3

/* Line Coding Structure */
#define LINE_CODING_LENGTH          0x07

/* Functional Descriptor Structure - See CDC Specification 1.1 for details */

// CDC header functional descriptor
typedef struct _USB_CDC_HEADER_FN_DSC {
    uint8_t     bFNLength;
    uint8_t     bDscType;
    uint8_t     bDscSubType;
    uint16_t    bcdCDC;
} __attribute__((packed)) USB_CDC_HEADER_FN_DSC;

// CDC Abstract Control Management Functional Descriptor
typedef struct _USB_CDC_ACM_FN_DSC {
    uint8_t     bFNLength;
    uint8_t     bDscType;
    uint8_t     bDscSubType;
    uint8_t     bmCapabilities;
} __attribute__((packed)) USB_CDC_ACM_FN_DSC;

// CDC Union Functional Descriptor
typedef struct _USB_CDC_UNION_FN_DSC {
    uint8_t     bFNLength;
    uint8_t     bDscType;
    uint8_t     bDscSubType;
    uint8_t     bMasterIntf;
    uint8_t     bSaveIntf0;
} __attribute__((packed)) USB_CDC_UNION_FN_DSC;

// CDC Call Management Functional Descriptor
typedef struct _USB_CDC_CALL_MGT_FN_DSC {
    uint8_t     bFNLength;
    uint8_t     bDscType;
    uint8_t     bDscSubType;
    uint8_t     bmCapabilities;
    uint8_t     bDataInterface;
} __attribute__((packed)) USB_CDC_CALL_MGT_FN_DSC;

