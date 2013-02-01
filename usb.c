//
// usb.c -- USB device mode support
//
//  Copyright (c) 2012-2013 Andrew Payne <andy@payne.org>
//


#include <stdio.h>
#include <string.h>
#include "freedom.h"
#include "common.h"
#include "usb.h"

// *******************************************************
//  TODO:  INCOMPLETE, work in progress
// *******************************************************

// USB Buffer table, the primary interface to the USB hardware module
typedef struct USB_BDT {
    union {
        volatile uint8_t _byte;
        struct {
            uint8_t    :2;
            uint8_t PID:4;
            uint8_t    :2;
        } PID;
    } stat;
    uint8_t     _dummy;
    uint16_t    count;
    uint8_t     *addr;             
} USB_BDT;

// Bit fields for BDT stat field
#define _BDT_STALL      (1 << 2)        // Issue STALL handshake
#define _DTS            (1 << 3)        // Enable data toggle synchronization
#define _NNIC           (1 << 4)        // Disable DMA address increment
#define _KEEP           (1 << 5)        // USB controller owns buffer forever
#define _DATA01         (1 << 6)        // DATA0/1 flag
#define _OWN            (1 << 7)        // USB controller owns buffer

// Token codes for PID field
#define SETUP_TOKEN    0x0D
#define OUT_TOKEN      0x01
#define IN_TOKEN       0x09

// Buffer descriptor table
#define MAX_ENDPOINTS 16
#define BDT_PER_EP 4
USB_BDT bdt[MAX_ENDPOINTS * BDT_PER_EP] __attribute__ ((aligned(512)));
static inline USB_BDT *bdt_rx(int num) { return &bdt[num * BDT_PER_EP];}
static inline USB_BDT *bdt_tx(int num) { return bdt_rx(num) + (BDT_PER_EP / 2);}

// Receive data buffers for endpoint 0
#define EP0_BUFSIZE 8
static uint8_t ep0_rx_buffers[2][EP0_BUFSIZE] __attribute__ ((aligned(4)));

typedef struct endpoint {
    uint8_t num;
    uint8_t data0;
    uint8_t tx_next;
    uint8_t tx_last;
    uint16_t pending_len;
    uint8_t *pending_data;
    void (*rx_handler)(struct endpoint *ep, uint8_t *data, int len);
} endpoint_t;
static endpoint_t endpoints[MAX_ENDPOINTS];

// Current USB device state
enum { POWER, ENUMERATED, ENABLED, ADDRESS, READY };
static int device_state;
static uint8_t device_address;

static const USB_DEV_DSC device_descriptor = {
    .bLength        = sizeof(USB_DEV_DSC),
    .bDscType       = mDEVICE,
    .bcdUSB         = 0x0200,
    .bDevCls        = 0x02,
    .bDevSubCls     = 0x00,
    .bDevProtocol   = 0x00,
    .bMaxPktSize0   = EP0_BUFSIZE,
    .idVendor       = 0xDEAD,
    .idProduct      = 0xBEAF,
    .bcdDevice      = 0x0000,
    .iMFR           = 0x01,         // Index to string Manufacturer descriptor
    .iProduct       = 0x02,         // Index to string product descriptor
    .iSerialNum     = 0x03,         // Index to string serial number
    .bNumCfg        = 0x01
};

#define CDC_STATUS_INTERFACE  0
#define CDC_DATA_INTERFACE    1
#define CDC_ACM_ENDPOINT      1
#define CDC_RX_ENDPOINT       2
#define CDC_TX_ENDPOINT       2
#define CDC_ACM_SIZE          16
#define CDC_RX_SIZE           64
#define CDC_TX_SIZE           64
#define NUM_INTERFACE         2


// Configuration descriptor
typedef struct USB_CONFIG {
    USB_CFG_DSC             i0;
    USB_INTF_DSC            i1;
    USB_CDC_HEADER_FN_DSC   i2;
    USB_CDC_CALL_MGT_FN_DSC i3;
    USB_CDC_ACM_FN_DSC      i4;
    USB_CDC_UNION_FN_DSC    i5;
    USB_EP_DSC              i6;
    USB_INTF_DSC            i7;
    USB_EP_DSC              i8;
    USB_EP_DSC              i9;
} __attribute__((packed)) USB_CONFIG;

static const USB_CONFIG config_descriptor = {
    {
        .bLength        = sizeof(USB_CFG_DSC),
        .bDscType       = mCONFIGURATION,
        .wTotalLength   = sizeof(USB_CONFIG),
        .bNumIntf       = 2,
        .bCfgValue      = 1,
        .iCfg           = 0,
        .bmAttributes   = 0xC0,
        .bMaxPower      = 0x32
    },{
        .bLength        = sizeof(USB_INTF_DSC),
        .bDscType       = mINTERFACE,
        .bIntfNum       = CDC_STATUS_INTERFACE,
        .bAltSetting    = 0,
        .bNumEPs        = 1,
        .bIntfCls       = 0x02,
        .bIntfSubCls    = 0x02,
        .bIntfProtocol  = 1,
        .iIntf          = 0
    },{
        .bFNLength      = sizeof(USB_CDC_HEADER_FN_DSC),
        .bDscType       = 0x24,
        .bDscSubType    = 0x00,
        .bcdCDC         = 0x0110
    },{
        .bFNLength      = sizeof(USB_CDC_CALL_MGT_FN_DSC), 
        .bDscType       = 0x24,
        .bDscSubType    = 0x01,
        .bmCapabilities = 0x00,
        .bDataInterface = 1 
    },{
        .bFNLength      = sizeof(USB_CDC_ACM_FN_DSC),
        .bDscType       = 0x24,
        .bDscSubType    = 0x02,
        .bmCapabilities = 0x06
    },{
        .bFNLength      = sizeof(USB_CDC_UNION_FN_DSC),
        .bDscType       = 0x24,
        .bDscSubType    = 0x06,
        .bMasterIntf    = CDC_STATUS_INTERFACE,
        .bSaveIntf0     = CDC_DATA_INTERFACE        
    },{
        .bLength        = sizeof(USB_EP_DSC),
        .bDscType       = mENDPOINT,
        .bEPAdr         = CDC_ACM_ENDPOINT | 0x80,
        .bmAttributes   = 0x03,
        .wMaxPktSize    = CDC_ACM_SIZE,
        .bInterval      = 64
    },{
        .bLength        = sizeof(USB_INTF_DSC),
        .bDscType       = mINTERFACE,
        .bIntfNum       = CDC_DATA_INTERFACE,
        .bAltSetting    = 0,
        .bNumEPs        = 2,
        .bIntfCls       = 0x0A,
        .bIntfSubCls    = 0x00,
        .bIntfProtocol  = 0,
        .iIntf          = 0 
    },{
        .bLength        = sizeof(USB_EP_DSC),
        .bDscType       = mENDPOINT,
        .bEPAdr         = CDC_RX_ENDPOINT,
        .bmAttributes   = 0x02,
        .wMaxPktSize    = CDC_RX_SIZE,
        .bInterval      = 0 
    },{
        .bLength        = sizeof(USB_EP_DSC),
        .bDscType       = mENDPOINT,
        .bEPAdr         = CDC_TX_ENDPOINT | 0x80,
        .bmAttributes   = 0x02,
        .wMaxPktSize    = CDC_TX_SIZE,
        .bInterval      = 0
    }       
};

// USB string tables
typedef struct usb_string {
    uint8_t bLength;
    uint8_t bDescriptorType;
    char data[];    
} usb_string;

// USB strings are UTF-16LE (little endian)
#define USB_STRING(str) { sizeof(str)-1+2, mSTRING, str }
static const usb_string manufacturer = USB_STRING("A\0P\0 \0C\0o\0n\0s\0u\0l\0t\0i\0n\0g\0 \0L\0L\0C\0");
static const usb_string product = USB_STRING("H\0A\0C\0K\0");
static const usb_string serial = USB_STRING("1\0");

const uint8_t string0[4] = {
    0x04,          // bLength
    0x03,          // bDescriptorType - STRING
    0x09, 0x04     // wLANGID[0] - English (American)
};

// Response table for GET_DESCRIPTION requests
typedef struct {
    uint16_t    wValue;
    uint8_t     *addr;
    uint16_t    length;
} usb_descriptor_list_t;

static const usb_descriptor_list_t usb_descriptor_list[] = {
    {0x0100, (uint8_t *)&device_descriptor, sizeof(device_descriptor)},
    {0x0200, (uint8_t *)&config_descriptor, sizeof(config_descriptor)},
    
    // Strings
    {0x0300, (uint8_t *)string0, 0},
    {0x0301, (uint8_t *)&manufacturer, 0},
    {0x0302, (uint8_t *)&product, 0},
    {0x0303, (uint8_t *)&serial, 0},
    {0x0000, 0, 0},                         // End marker
};

// -----------------------------------------------------------------------------------

inline int min(int a, int b)
{
    if(a < b)
        return a;
    else
        return b;
}

// -----------------------------------------------------------------------------------

void usb_dump(void)
{
    int i;
    
    iprintf("USB status:\r\n");
    iprintf("USB0_OTGSTAT=0x%x, OTGISTAT=0x%x, STAT=0x%x\r\n", USB0_OTGSTAT,
                     USB0_OTGISTAT, USB0_STAT);
    iprintf("USB0_ERRSTAT=0x%02x\r\n", USB0_ERRSTAT);
    iprintf("USB0_CTL=0x%02x\r\n", USB0_CTL);
    iprintf("USB0_ISTAT=0x%02x\r\n", USB0_ISTAT);
    iprintf("USB0_ADDR=0x%02x\r\n", USB0_ADDR);
    for(i=0; i<4; i++) {
        iprintf("  stat=0x%02x, len=%d\r\n", bdt[i].stat._byte, bdt[i].count);
    }   
}

// -----------------------------------------------------------------------------------

void usb_init(void)
{
    device_state = POWER;
    
    // Enable USB clocks
    SIM_SOPT2 |= SIM_SOPT2_USBSRC_MASK;
    SIM_SCGC4 |= SIM_SCGC4_USBOTG_MASK;

    // Reset USB module
    USB0_USBTRC0 |= USB_USBTRC0_USBRESET_MASK;
    while((USB0_USBTRC0 & USB_USBTRC0_USBRESET_MASK) != 0)
        ;
    
    // Set the Buffer Descriptor Register address
    USB0_BDTPAGE1 = (uint8_t)((uint32_t)bdt >> 8);
    USB0_BDTPAGE2 = (uint8_t)((uint32_t)bdt >> 16);
    USB0_BDTPAGE3 = (uint8_t)((uint32_t)bdt >> 24);
    
    // Clear any pending interrupts, and enable just the reset interrupt
    USB0_ISTAT = 0xff;
    USB0_INTEN = USB_INTEN_USBRSTEN_MASK;
    
    // Disable weak pull downs, take out of suspend state
    USB0_USBCTRL = 0;
    USB0_CONTROL = USB_CONTROL_DPPULLUPNONOTG_MASK; // Eanble pullup in non-OTG mode
    USB0_USBTRC0 |= 0x40;   // "Software must set this bit to one"
    
    // Enable USB module, and enable interrupts
    USB0_CTL = USB_CTL_USBENSOFEN_MASK;
    enable_irq(INT_USB0);
}

// Clear any pending tx packets for a given endpoint
static void ep_clear_tx(endpoint_t *ep, int tx_last)
{
    USB_BDT *bdtptr = bdt_tx(ep->num);
    bdtptr[0].stat._byte = bdtptr[1].stat._byte = 0;
    ep->tx_last = tx_last;
    ep->tx_next = !tx_last;
}

// Initialize/enable an endpoint
static void usb_init_ep(int num, int buflen, uint8_t *rx_buf0, uint8_t *rx_buf1)
{
    endpoint_t *ep = &endpoints[num];

    ep->num = num;
    ep_clear_tx(ep, 1);
    
    // Configure BDT entries for receive
    USB_BDT *bdtptr = bdt_rx(num);
    bdtptr[0].addr = rx_buf0;
    bdtptr[1].addr = rx_buf1;
    bdtptr[0].count = bdtptr[1].count = buflen;
    bdtptr[0].stat._byte = bdtptr[1].stat._byte = _OWN;
    
    // Enable endpoint for transmit and receive
    USB0_ENDPT(num) = USB_ENDPT_EPTXEN_MASK | USB_ENDPT_EPRXEN_MASK 
                        | USB_ENDPT_EPHSHK_MASK;
}

static void usb_reset(void)
{
    USB0_CTL |= USB_CTL_ODDRST_MASK;

    // Configure endpoint 0 (the control endpoint)
    usb_init_ep(0, EP0_BUFSIZE, ep0_rx_buffers[0], ep0_rx_buffers[1]);

    // Clear all error and interrupt flags
    USB0_ERRSTAT = 0xFF;
    USB0_ISTAT = 0xFF;

    // Set default USB address
    USB0_ADDR = 0x00;

    // Enable all error sources
    USB0_ERREN = 0x7E;

    // Enable USB interrupts
    USB0_INTEN = USB_INTEN_TOKDNEEN_MASK | USB_INTEN_ERROREN_MASK 
                    | USB_INTEN_USBRSTEN_MASK | USB_INTEN_STALLEN_MASK;
}

// Get BDT for next available TX buffer for endpoint
inline static USB_BDT *ep_next_tx(endpoint_t *ep)
{
    return bdt_tx(ep->num) + ep->tx_next;
}

// Send a bufffer for a given endpoint
static int usb_tx(endpoint_t *ep, uint8_t *data, int len)
{
    USB_BDT *bdt_ptr = ep_next_tx(ep);

    // Check to see if the next tx buffer is free   
    if (bdt_ptr->stat._byte & _OWN)
        return 0;
        
    bdt_ptr->count = len;
    bdt_ptr->addr = data;
    bdt_ptr->stat._byte = _OWN | ep->data0;     // Give to USB controller
    
    ep->tx_next ^= 1;                           // Alternate transmit buffers
    ep->data0 ^= _DATA01;                       // Alternate DATA0/1
    
    return len;
}

static void usb_tx_handler(endpoint_t *ep, USB_BDT *bdt_ptr)
{
    int len;

    // Queue any pending data transfers
    while(ep->pending_len > 0) {
        len = min(ep->pending_len, EP0_BUFSIZE);
        if(!usb_tx(ep, ep->pending_data, len))          // No more transmit buffers
            break;
    
        ep->pending_len -= len;
        ep->pending_data += len;
    }
}

// Queue a buffer for sending over a given endpoint
static void usb_queue_tx(endpoint_t *ep, uint8_t *data, int len)
{
    ep->pending_data = data;
    ep->pending_len = len;
    usb_tx_handler(ep, NULL);
}

static void cdc_rx_handler(endpoint_t *ep, uint8_t *data, int len)
{
    // TODO:  proper echo handler without a buffer race
    usb_tx(ep, data, len);
}

// TODO:  move this to a CDC-specific file
static void usb_set_config(uint16_t value)
{
    static uint8_t ep1_rx_buffers[2][CDC_ACM_SIZE] __attribute__ ((aligned(4)));
    static uint8_t ep2_rx_buffers[2][CDC_RX_SIZE] __attribute__ ((aligned(4)));
    
    usb_init_ep(1, CDC_ACM_SIZE, ep1_rx_buffers[0], ep1_rx_buffers[1]);
    usb_init_ep(2, CDC_RX_SIZE, ep2_rx_buffers[0], ep2_rx_buffers[1]);
    endpoints[2].rx_handler = cdc_rx_handler;
}

static void usb_setup_device(endpoint_t *ep, USB_SETUP *setup)
{
    const usb_descriptor_list_t *p;
    int len;

    switch (setup->bRequest) {
        case mGET_DESC:
            p = usb_descriptor_list;                    // Find entry in table
            while(p->wValue) {
                if(p->wValue == setup->wValue) {
                    if(p->length == 0)
                        len = p->addr[0];               // Use structure length
                    else
                        len = p->length;
                        
                    iprintf("sending 0x%04x %d\r\n",setup->wValue, len);
                    usb_queue_tx(ep, p->addr, min(len, setup->wLength));
                    return;
                }
                p++;
            }
            iprintf("NOT IMPLEMENTED! 0x%04x\r\n", setup->wValue);              
            break;
            
        case mSET_ADDRESS:
            device_state = ADDRESS;
            device_address = setup->wValue & 0x7f;
            usb_tx(ep,0,0);                         // Send handshake
            break;
            
        case mSET_CONFIG:
            iprintf("setconfig: %d\r\n", setup->wValue);
            device_state = ENUMERATED;
            usb_set_config(setup->wValue);
            usb_tx(ep,0,0);                         // Send handshake
            break;
            
        default:
            iprintf("NOT IMPLEMENTED! %d\r\n", setup->bRequest);
            break;
    }
}

// XXX:  move this to serial/CDC-specific file

typedef struct {
    uint32_t  DTERate;
    uint8_t   CharFormat;
    uint8_t   ParityType;
    uint8_t   Databits;
} cdc_line_coding_t;

static cdc_line_coding_t line_coding;

static void rx_send_handshake(endpoint_t *ep, uint8_t *data, int len)
{
    // NOTE:  Receive data is ignored
    usb_tx(ep,0,0);                             // Send handshake
    ep->rx_handler = NULL;  
}

static void usb_setup_interface(endpoint_t *ep, USB_SETUP *setup)
{
    switch(setup->bRequest) {
        case GET_LINE_CODING:
            usb_queue_tx(ep, (uint8_t *) &line_coding, sizeof(line_coding));
            break;
            
        case SET_LINE_CODING:
            ep->rx_handler = rx_send_handshake;
            break;
            
        case SET_CONTROL_LINE_STATE:
            usb_tx(ep,0,0);             // XXX: check this
            break;
            
        default:
            iprintf("setup_interface: %d\r\n", setup->bRequest);
            break;      
    }   
}

static void usb_setup_endpoint(endpoint_t *ep, USB_SETUP *setup)
{
    iprintf("setup_endpoint\r\n");      
}

static void usb_handler(uint8_t stat)
{
    unsigned int i = stat >> 2;
    USB_BDT *bdt_ptr = &bdt[i]; 
    endpoint_t *ep = &endpoints[i >> 2];
        
    switch(bdt_ptr->stat.PID.PID) {
        case OUT_TOKEN:
            if(ep->rx_handler)
                (*(ep->rx_handler))(ep, bdt_ptr->addr, bdt_ptr->count);
            break;

        case IN_TOKEN:
            usb_tx_handler(ep, bdt_ptr);
            if(device_state == ADDRESS) {
                USB0_ADDR = device_address;
                iprintf("USB0_ADDR = %d\r\n", USB0_ADDR);
                device_state = READY;       
            }
            ep->tx_last = i & 1;            // Save even/odd of last buffer sent
            break;
            
        case SETUP_TOKEN:
            ep->data0 = _DATA01;            // Setup is always DATA1
            ep_clear_tx(ep, ep->tx_last);
            USB_SETUP *setup = (USB_SETUP*) bdt_ptr->addr;
            switch(setup->bmRequestType & 0x1f) {
                case 0:     usb_setup_device(ep, setup);        break;
                case 1:     usb_setup_interface(ep, setup);     break;
                case 2:     usb_setup_endpoint(ep, setup);      break;
                default:                                        break;
            }
            USB0_CTL = USB_CTL_USBENSOFEN_MASK;  // Clear TXSUSPENDTOKENBUSY
            break;
    }

    // For receive buffers, configure to receive next token
    int tx = stat & 0x8;
    if(!tx) {
        bdt_ptr->count = EP0_BUFSIZE;               // XXX: generalize this
        bdt_ptr->stat._byte = _OWN;
    }
}

void USBOTG_IRQHandler(void) 
{
    uint8_t istat = USB0_ISTAT;
    
    if(istat & USB_ISTAT_USBRST_MASK) {         // Reset
        usb_reset();
        return;
    }

    // Process any pending token done interrupts (may be queued)
    while(istat & USB_ISTAT_TOKDNE_MASK) {
        usb_handler(USB0_STAT);
        USB0_ISTAT = USB_ISTAT_TOKDNE_MASK;
        istat = USB0_ISTAT;
    }
        
    if(istat & USB_ISTAT_STALL_MASK) {
        USB0_ENDPT0 &= ~USB_ENDPT_EPSTALL_MASK;
        USB0_ISTAT = USB_ISTAT_STALL_MASK;
    }
    
    if(istat & USB_ISTAT_ERROR_MASK) {
        iprintf("USB error: 0x%x\r\n", USB0_ERRSTAT);
        USB0_ISTAT = USB_ISTAT_ERROR_MASK;
        USB0_INTEN = 0;                             // Disable all USB interrupts
        return;
    }
}