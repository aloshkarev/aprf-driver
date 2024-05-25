#ifndef ELTEX_WIFI_HWSIM_H
#define ELTEX_WIFI_HWSIM_H
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <linux/rhashtable.h>
#include <linux/nospec.h>
#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/dynamic_debug.h>

#define netdev_set_def_destructor(_dev) (_dev)->needs_free_netdev = true;


#define CHAN2G(_freq)  { \
	.band = NL80211_BAND_2GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
}

#define CHAN5G(_freq) { \
	.band = NL80211_BAND_5GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
}

#define CHAN6G(_freq) { \
	.band = NL80211_BAND_6GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
}

/**
 * enum hwsim_tx_control_flags - flags to describe transmission info/status
 *
 * These flags are used to give the wmediumd extra information in order to
 * modify its behavior for each frame
 *
 * @HWSIM_TX_CTL_REQ_TX_STATUS: require TX status callback for this frame.
 * @HWSIM_TX_CTL_NO_ACK: tell the wmediumd not to wait for an ack
 * @HWSIM_TX_STAT_ACK: Frame was acknowledged
 *
 */
enum hwsim_tx_control_flags {
    HWSIM_TX_CTL_REQ_TX_STATUS		= BIT(0),
    HWSIM_TX_CTL_NO_ACK			= BIT(1),
    HWSIM_TX_STAT_ACK			= BIT(2),
};

/**
 * DOC: Frame transmission/registration support
 *
 * Frame transmission and registration support exists to allow userspace
 * entities such as wmediumd to receive and process all broadcasted
 * frames from a eltex_wifi_emu radio device.
 *
 * This allow user space applications to decide if the frame should be
 * dropped or not and implement a wireless medium simulator at user space.
 *
 * Registration is done by sending a register message to the driver and
 * will be automatically unregistered if the user application doesn't
 * responds to sent frames.
 * Once registered the user application has to take responsibility of
 * broadcasting the frames to all listening aprf_drv radio
 * interfaces.
 *
 * For more technical details, see the corresponding command descriptions
 * below.
 */

/**
 * enum hwsim_commands - supported hwsim commands
 *
 * @HWSIM_CMD_UNSPEC: unspecified command to catch errors
 *
 * @HWSIM_CMD_REGISTER: request to register and received all broadcasted
 *	frames by any aprf_drv radio device.
 * @HWSIM_CMD_FRAME: send/receive a broadcasted frame from/to kernel/user
 *	space, uses:
 *	%HWSIM_ATTR_ADDR_TRANSMITTER, %HWSIM_ATTR_ADDR_RECEIVER,
 *	%HWSIM_ATTR_FRAME, %HWSIM_ATTR_FLAGS, %HWSIM_ATTR_RX_RATE,
 *	%HWSIM_ATTR_SIGNAL, %HWSIM_ATTR_COOKIE, %HWSIM_ATTR_FREQ (optional)
 * @HWSIM_CMD_TX_INFO_FRAME: Transmission info report from user space to
 *	kernel, uses:
 *	%HWSIM_ATTR_ADDR_TRANSMITTER, %HWSIM_ATTR_FLAGS,
 *	%HWSIM_ATTR_TX_INFO, %WSIM_ATTR_TX_INFO_FLAGS,
 *	%HWSIM_ATTR_SIGNAL, %HWSIM_ATTR_COOKIE
 * @HWSIM_CMD_NEW_RADIO: create a new radio with the given parameters,
 *	returns the radio ID (>= 0) or negative on errors, if successful
 *	then multicast the result, uses optional parameter:
 *	%HWSIM_ATTR_REG_STRICT_REG, %HWSIM_ATTR_SUPPORT_P2P_DEVICE,
 *	%HWSIM_ATTR_DESTROY_RADIO_ON_CLOSE, %HWSIM_ATTR_CHANNELS,
 *	%HWSIM_ATTR_NO_VIF, %HWSIM_ATTR_RADIO_NAME, %HWSIM_ATTR_USE_CHANCTX,
 *	%HWSIM_ATTR_REG_HINT_ALPHA2, %HWSIM_ATTR_REG_CUSTOM_REG,
 *	%HWSIM_ATTR_PERM_ADDR
 * @HWSIM_CMD_DEL_RADIO: destroy a radio, reply is multicasted
 * @HWSIM_CMD_GET_RADIO: fetch information about existing radios, uses:
 *	%HWSIM_ATTR_RADIO_ID
 * @HWSIM_CMD_ADD_MAC_ADDR: add a receive MAC address (given in the
 *	%HWSIM_ATTR_ADDR_RECEIVER attribute) to a device identified by
 *	%HWSIM_ATTR_ADDR_TRANSMITTER. This lets wmediumd forward frames
 *	to this receiver address for a given station.
 * @HWSIM_CMD_DEL_MAC_ADDR: remove the MAC address again, the attributes
 *	are the same as to @HWSIM_CMD_ADD_MAC_ADDR.
 * @__HWSIM_CMD_MAX: enum limit
 */
enum {
    HWSIM_CMD_UNSPEC,
    HWSIM_CMD_REGISTER,
    HWSIM_CMD_FRAME,
    HWSIM_CMD_TX_INFO_FRAME,
    HWSIM_CMD_NEW_RADIO,
    HWSIM_CMD_DEL_RADIO,
    HWSIM_CMD_GET_RADIO,
    HWSIM_CMD_ADD_MAC_ADDR,
    HWSIM_CMD_DEL_MAC_ADDR,
    HWSIM_CMD_START_PMSR,
	HWSIM_CMD_ABORT_PMSR,
	HWSIM_CMD_REPORT_PMSR,
    __HWSIM_CMD_MAX,
};
#define HWSIM_CMD_MAX (_HWSIM_CMD_MAX - 1)

#define HWSIM_CMD_CREATE_RADIO   HWSIM_CMD_NEW_RADIO
#define HWSIM_CMD_DESTROY_RADIO  HWSIM_CMD_DEL_RADIO

/**
 * enum hwsim_attrs - hwsim netlink attributes
 *
 * @HWSIM_ATTR_UNSPEC: unspecified attribute to catch errors
 *
 * @HWSIM_ATTR_ADDR_RECEIVER: MAC address of the radio device that
 *	the frame is broadcasted to
 * @HWSIM_ATTR_ADDR_TRANSMITTER: MAC address of the radio device that
 *	the frame was broadcasted from
 * @HWSIM_ATTR_FRAME: Data array
 * @HWSIM_ATTR_FLAGS: mac80211 transmission flags, used to process
	properly the frame at user space
 * @HWSIM_ATTR_RX_RATE: estimated rx rate index for this frame at user
	space
 * @HWSIM_ATTR_SIGNAL: estimated RX signal for this frame at user
	space
 * @HWSIM_ATTR_TX_INFO: ieee80211_tx_rate array
 * @HWSIM_ATTR_COOKIE: sk_buff cookie to identify the frame
 * @HWSIM_ATTR_CHANNELS: u32 attribute used with the %HWSIM_CMD_CREATE_RADIO
 *	command giving the number of channels supported by the new radio
 * @HWSIM_ATTR_RADIO_ID: u32 attribute used with %HWSIM_CMD_DESTROY_RADIO
 *	only to destroy a radio
 * @HWSIM_ATTR_REG_HINT_ALPHA2: alpha2 for regulatoro driver hint
 *	(nla string, length 2)
 * @HWSIM_ATTR_REG_CUSTOM_REG: custom regulatory domain index (u32 attribute)
 * @HWSIM_ATTR_REG_STRICT_REG: request REGULATORY_STRICT_REG (flag attribute)
 * @HWSIM_ATTR_SUPPORT_P2P_DEVICE: support P2P Device virtual interface (flag)
 * @HWSIM_ATTR_USE_CHANCTX: used with the %HWSIM_CMD_CREATE_RADIO
 *	command to force use of channel contexts even when only a
 *	single channel is supported
 * @HWSIM_ATTR_DESTROY_RADIO_ON_CLOSE: used with the %HWSIM_CMD_CREATE_RADIO
 *	command to force radio removal when process that created the radio dies
 * @HWSIM_ATTR_RADIO_NAME: Name of radio, e.g. phy666
 * @HWSIM_ATTR_NO_VIF:  Do not create vif (wlanX) when creating radio.
 * @HWSIM_ATTR_FREQ: Frequency at which packet is transmitted or received.
 * @HWSIM_ATTR_TX_INFO_FLAGS: additional flags for corresponding
 *	rates of %HWSIM_ATTR_TX_INFO
 * @HWSIM_ATTR_PERM_ADDR: permanent mac address of new radio
 * @HWSIM_ATTR_IFTYPE_SUPPORT: u32 attribute of supported interface types bits
 * @HWSIM_ATTR_CIPHER_SUPPORT: u32 array of supported cipher types
 * @__HWSIM_ATTR_MAX: enum limit
 */


enum {
    HWSIM_ATTR_UNSPEC,
    HWSIM_ATTR_ADDR_RECEIVER,
    HWSIM_ATTR_ADDR_TRANSMITTER,
    HWSIM_ATTR_FRAME,
    HWSIM_ATTR_FLAGS,
    HWSIM_ATTR_RX_RATE,
    HWSIM_ATTR_SIGNAL,
    HWSIM_ATTR_TX_INFO,
    HWSIM_ATTR_COOKIE,
    HWSIM_ATTR_CHANNELS,
    HWSIM_ATTR_RADIO_ID,
    HWSIM_ATTR_REG_HINT_ALPHA2,
    HWSIM_ATTR_REG_CUSTOM_REG,
    HWSIM_ATTR_REG_STRICT_REG,
    HWSIM_ATTR_SUPPORT_P2P_DEVICE,
    HWSIM_ATTR_USE_CHANCTX,
    HWSIM_ATTR_DESTROY_RADIO_ON_CLOSE,
    HWSIM_ATTR_RADIO_NAME,
    HWSIM_ATTR_NO_VIF,
    HWSIM_ATTR_FREQ,
    HWSIM_ATTR_PAD,
    HWSIM_ATTR_TX_INFO_FLAGS,
    HWSIM_ATTR_PERM_ADDR,
    HWSIM_ATTR_IFTYPE_SUPPORT,
    HWSIM_ATTR_CIPHER_SUPPORT,
	HWSIM_ATTR_MLO_SUPPORT,
	HWSIM_ATTR_PMSR_SUPPORT,
	HWSIM_ATTR_PMSR_REQUEST,
	HWSIM_ATTR_PMSR_RESULT,
    __HWSIM_ATTR_MAX,
};
#define HWSIM_ATTR_MAX (__HWSIM_ATTR_MAX - 1)

/**
 * struct hwsim_tx_rate - rate selection/status
 *
 * @idx: rate index to attempt to send with
 * @count: number of tries in this rate before going to the next rate
 *
 * A value of -1 for @idx indicates an invalid rate and, if used
 * in an array of retry rates, that no more rates should be tried.
 *
 * When used for transmit status reporting, the driver should
 * always report the rate and number of retries used.
 *
 */
struct hwsim_tx_rate {
    s8 idx;
    u8 count;
} __packed;

/**
 * enum hwsim_tx_rate_flags - per-rate flags set by the rate control algorithm.
 *	Inspired by structure mac80211_rate_control_flags. New flags may be
 *	appended, but old flags not deleted, to keep compatibility for
 *	userspace.
 *
 * These flags are set by the Rate control algorithm for each rate during tx,
 * in the @flags member of struct ieee80211_tx_rate.
 *
 * @WIFI_HWSIM_TX_RC_USE_RTS_CTS: Use RTS/CTS exchange for this rate.
 * @WIFI_HWSIM_TX_RC_USE_CTS_PROTECT: CTS-to-self protection is required.
 *	This is set if the current BSS requires ERP protection.
 * @WIFI_HWSIM_TX_RC_USE_SHORT_PREAMBLE: Use short preamble.
 * @WIFI_HWSIM_TX_RC_MCS: HT rate.
 * @WIFI_HWSIM_TX_RC_VHT_MCS: VHT MCS rate, in this case the idx field is
 *	split into a higher 4 bits (Nss) and lower 4 bits (MCS number)
 * @WIFI_HWSIM_TX_RC_GREEN_FIELD: Indicates whether this rate should be used
 *	in Greenfield mode.
 * @WIFI_HWSIM_TX_RC_40_MHZ_WIDTH: Indicates if the Channel Width should be
 *	40 MHz.
 * @WIFI_HWSIM_TX_RC_80_MHZ_WIDTH: Indicates 80 MHz transmission
 * @WIFI_HWSIM_TX_RC_160_MHZ_WIDTH: Indicates 160 MHz transmission
 *	(80+80 isn't supported yet)
 * @WIFI_HWSIM_TX_RC_DUP_DATA: The frame should be transmitted on both of
 *	the adjacent 20 MHz channels, if the current channel type is
 *	NL80211_CHAN_HT40MINUS or NL80211_CHAN_HT40PLUS.
 * @WIFI_HWSIM_TX_RC_SHORT_GI: Short Guard interval should be used for this
 *	rate.
 */
enum hwsim_tx_rate_flags {
    WIFI_HWSIM_TX_RC_USE_RTS_CTS		= BIT(0),
    WIFI_HWSIM_TX_RC_USE_CTS_PROTECT		= BIT(1),
    WIFI_HWSIM_TX_RC_USE_SHORT_PREAMBLE	= BIT(2),

    /* rate index is an HT/VHT MCS instead of an index */
    WIFI_HWSIM_TX_RC_MCS			= BIT(3),
    WIFI_HWSIM_TX_RC_GREEN_FIELD		= BIT(4),
    WIFI_HWSIM_TX_RC_40_MHZ_WIDTH		= BIT(5),
    WIFI_HWSIM_TX_RC_DUP_DATA		= BIT(6),
    WIFI_HWSIM_TX_RC_SHORT_GI		= BIT(7),
    WIFI_HWSIM_TX_RC_VHT_MCS			= BIT(8),
    WIFI_HWSIM_TX_RC_80_MHZ_WIDTH		= BIT(9),
    WIFI_HWSIM_TX_RC_160_MHZ_WIDTH		= BIT(10),
};

/**
 * struct hwsim_tx_rate - rate selection/status
 *
 * @idx: rate index to attempt to send with
 * @count: number of tries in this rate before going to the next rate
 *
 * A value of -1 for @idx indicates an invalid rate and, if used
 * in an array of retry rates, that no more rates should be tried.
 *
 * When used for transmit status reporting, the driver should
 * always report the rate and number of retries used.
 *
 */
struct hwsim_tx_rate_flag {
    s8 idx;
    u16 flags;
} __packed;

/**
 * DOC: Frame transmission support over virtio
 *
 * Frame transmission is also supported over virtio to allow communication
 * with external entities.
 */

/**
 * enum hwsim_vqs - queues for virtio frame transmission
 *
 * @HWSIM_VQ_TX: send frames to external entity
 * @HWSIM_VQ_RX: receive frames and transmission info reports
 * @HWSIM_NUM_VQS: enum limit
 */
enum {
    HWSIM_VQ_TX,
    HWSIM_VQ_RX,
    HWSIM_NUM_VQS,
};


/**
 * enum hwsim_regtest - the type of regulatory tests we offer
 *
 * These are the different values you can use for the regtest
 * module parameter. This is useful to help test world roaming
 * and the driver regulatory_hint() call and combinations of these.
 * If you want to do specific alpha2 regulatory domain tests simply
 * use the userspace regulatory request as that will be respected as
 * well without the need of this module parameter. This is designed
 * only for testing the driver regulatory request, world roaming
 * and all possible combinations.
 *
 * @HWSIM_REGTEST_DISABLED: No regulatory tests are performed,
 * 	this is the default value.
 * @HWSIM_REGTEST_DRIVER_REG_FOLLOW: Used for testing the driver regulatory
 *	hint, only one driver regulatory hint will be sent as such the
 * 	secondary radios are expected to follow.
 * @HWSIM_REGTEST_DRIVER_REG_ALL: Used for testing the driver regulatory
 * 	request with all radios reporting the same regulatory domain.
 * @HWSIM_REGTEST_DIFF_COUNTRY: Used for testing the drivers calling
 * 	different regulatory domains requests. Expected behaviour is for
 * 	an intersection to occur but each device will still use their
 * 	respective regulatory requested domains. Subsequent radios will
 * 	use the resulting intersection.
 * @HWSIM_REGTEST_WORLD_ROAM: Used for testing the world roaming. We accomplish
 *	this by using a custom beacon-capable regulatory domain for the first
 *	radio. All other device world roam.
 * @HWSIM_REGTEST_CUSTOM_WORLD: Used for testing the custom world regulatory
 * 	domain requests. All radios will adhere to this custom world regulatory
 * 	domain.
 * @HWSIM_REGTEST_CUSTOM_WORLD_2: Used for testing 2 custom world regulatory
 * 	domain requests. The first radio will adhere to the first custom world
 * 	regulatory domain, the second one to the second custom world regulatory
 * 	domain. All other devices will world roam.
 * @HWSIM_REGTEST_STRICT_FOLLOW: Used for testing strict regulatory domain
 *	settings, only the first radio will send a regulatory domain request
 *	and use strict settings. The rest of the radios are expected to follow.
 * @HWSIM_REGTEST_STRICT_ALL: Used for testing strict regulatory domain
 *	settings. All radios will adhere to this.
 * @HWSIM_REGTEST_STRICT_AND_DRIVER_REG: Used for testing strict regulatory
 *	domain settings, combined with secondary driver regulatory domain
 *	settings. The first radio will get a strict regulatory domain setting
 *	using the first driver regulatory request and the second radio will use
 *	non-strict settings using the second driver regulatory request. All
 *	other devices should follow the intersection created between the
 *	first two.
 * @HWSIM_REGTEST_ALL: Used for testing every possible mix. You will need
 * 	at least 6 radios for a complete test. We will test in this order:
 * 	1 - driver custom world regulatory domain
 * 	2 - second custom world regulatory domain
 * 	3 - first driver regulatory domain request
 * 	4 - second driver regulatory domain request
 * 	5 - strict regulatory domain settings using the third driver regulatory
 * 	    domain request
 * 	6 and on - should follow the intersection of the 3rd, 4rth and 5th radio
 * 	           regulatory requests.
 */
enum hwsim_regtest {
    HWSIM_REGTEST_DISABLED = 0,
    HWSIM_REGTEST_DRIVER_REG_FOLLOW = 1,
    HWSIM_REGTEST_DRIVER_REG_ALL = 2,
    HWSIM_REGTEST_DIFF_COUNTRY = 3,
    HWSIM_REGTEST_WORLD_ROAM = 4,
    HWSIM_REGTEST_CUSTOM_WORLD = 5,
    HWSIM_REGTEST_CUSTOM_WORLD_2 = 6,
    HWSIM_REGTEST_STRICT_FOLLOW = 7,
    HWSIM_REGTEST_STRICT_ALL = 8,
    HWSIM_REGTEST_STRICT_AND_DRIVER_REG = 9,
    HWSIM_REGTEST_ALL = 10,
};


static const u32 hwsim_ciphers[] = {
        WLAN_CIPHER_SUITE_WEP40,
        WLAN_CIPHER_SUITE_WEP104,
        WLAN_CIPHER_SUITE_TKIP,
        WLAN_CIPHER_SUITE_CCMP,
        WLAN_CIPHER_SUITE_CCMP_256,
        WLAN_CIPHER_SUITE_GCMP,
        WLAN_CIPHER_SUITE_GCMP_256,
        WLAN_CIPHER_SUITE_AES_CMAC,
        WLAN_CIPHER_SUITE_BIP_CMAC_256,
        WLAN_CIPHER_SUITE_BIP_GMAC_128,
        WLAN_CIPHER_SUITE_BIP_GMAC_256,
};


static const struct ieee80211_rate hwsim_rates[] = {
        { .bitrate = 10 },
        { .bitrate = 20, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
        { .bitrate = 55, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
        { .bitrate = 110, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
        { .bitrate = 60 },
        { .bitrate = 90 },
        { .bitrate = 120 },
        { .bitrate = 180 },
        { .bitrate = 240 },
        { .bitrate = 360 },
        { .bitrate = 480 },
        { .bitrate = 540 }
};

#define NUM_S1G_CHANS_US 51
static struct ieee80211_channel hwsim_channels_s1g[NUM_S1G_CHANS_US];

static const struct ieee80211_channel hwsim_channels_2ghz[] = {
        CHAN2G(2412), /* Channel 1 */
        CHAN2G(2417), /* Channel 2 */
        CHAN2G(2422), /* Channel 3 */
        CHAN2G(2427), /* Channel 4 */
        CHAN2G(2432), /* Channel 5 */
        CHAN2G(2437), /* Channel 6 */
        CHAN2G(2442), /* Channel 7 */
        CHAN2G(2447), /* Channel 8 */
        CHAN2G(2452), /* Channel 9 */
        CHAN2G(2457), /* Channel 10 */
        CHAN2G(2462), /* Channel 11 */
        CHAN2G(2467), /* Channel 12 */
        CHAN2G(2472), /* Channel 13 */
        CHAN2G(2484), /* Channel 14 */
};

static const struct ieee80211_channel hwsim_channels_5ghz[] = {
        CHAN5G(5180), /* Channel 36 */
        CHAN5G(5200), /* Channel 40 */
        CHAN5G(5220), /* Channel 44 */
        CHAN5G(5240), /* Channel 48 */

        CHAN5G(5260), /* Channel 52 */
        CHAN5G(5280), /* Channel 56 */
        CHAN5G(5300), /* Channel 60 */
        CHAN5G(5320), /* Channel 64 */

        CHAN5G(5500), /* Channel 100 */
        CHAN5G(5520), /* Channel 104 */
        CHAN5G(5540), /* Channel 108 */
        CHAN5G(5560), /* Channel 112 */
        CHAN5G(5580), /* Channel 116 */
        CHAN5G(5600), /* Channel 120 */
        CHAN5G(5620), /* Channel 124 */
        CHAN5G(5640), /* Channel 128 */
        CHAN5G(5660), /* Channel 132 */
        CHAN5G(5680), /* Channel 136 */
        CHAN5G(5700), /* Channel 140 */

        CHAN5G(5745), /* Channel 149 */
        CHAN5G(5765), /* Channel 153 */
        CHAN5G(5785), /* Channel 157 */
        CHAN5G(5805), /* Channel 161 */
        CHAN5G(5825), /* Channel 165 */
        CHAN5G(5845), /* Channel 169 */

        CHAN5G(5855), /* Channel 171 */
        CHAN5G(5860), /* Channel 172 */
        CHAN5G(5865), /* Channel 173 */
        CHAN5G(5870), /* Channel 174 */

        CHAN5G(5875), /* Channel 175 */
        CHAN5G(5880), /* Channel 176 */
        CHAN5G(5885), /* Channel 177 */
        CHAN5G(5890), /* Channel 178 */
        CHAN5G(5895), /* Channel 179 */
        CHAN5G(5900), /* Channel 180 */
        CHAN5G(5905), /* Channel 181 */

        CHAN5G(5910), /* Channel 182 */
        CHAN5G(5915), /* Channel 183 */
        CHAN5G(5920), /* Channel 184 */
        CHAN5G(5925), /* Channel 185 */
};

static const struct ieee80211_channel hwsim_channels_6ghz[] = {
        CHAN6G(5955), /* Channel 1 */
        CHAN6G(5975), /* Channel 5 */
        CHAN6G(5995), /* Channel 9 */
        CHAN6G(6015), /* Channel 13 */
        CHAN6G(6035), /* Channel 17 */
        CHAN6G(6055), /* Channel 21 */
        CHAN6G(6075), /* Channel 25 */
        CHAN6G(6095), /* Channel 29 */
        CHAN6G(6115), /* Channel 33 */
        CHAN6G(6135), /* Channel 37 */
        CHAN6G(6155), /* Channel 41 */
        CHAN6G(6175), /* Channel 45 */
        CHAN6G(6195), /* Channel 49 */
        CHAN6G(6215), /* Channel 53 */
        CHAN6G(6235), /* Channel 57 */
        CHAN6G(6255), /* Channel 61 */
        CHAN6G(6275), /* Channel 65 */
        CHAN6G(6295), /* Channel 69 */
        CHAN6G(6315), /* Channel 73 */
        CHAN6G(6335), /* Channel 77 */
        CHAN6G(6355), /* Channel 81 */
        CHAN6G(6375), /* Channel 85 */
        CHAN6G(6395), /* Channel 89 */
        CHAN6G(6415), /* Channel 93 */
        CHAN6G(6435), /* Channel 97 */
        CHAN6G(6455), /* Channel 181 */
        CHAN6G(6475), /* Channel 105 */
        CHAN6G(6495), /* Channel 109 */
        CHAN6G(6515), /* Channel 113 */
        CHAN6G(6535), /* Channel 117 */
        CHAN6G(6555), /* Channel 121 */
        CHAN6G(6575), /* Channel 125 */
        CHAN6G(6595), /* Channel 129 */
        CHAN6G(6615), /* Channel 133 */
        CHAN6G(6635), /* Channel 137 */
        CHAN6G(6655), /* Channel 141 */
        CHAN6G(6675), /* Channel 145 */
        CHAN6G(6695), /* Channel 149 */
        CHAN6G(6715), /* Channel 153 */
        CHAN6G(6735), /* Channel 157 */
        CHAN6G(6755), /* Channel 161 */
        CHAN6G(6775), /* Channel 165 */
        CHAN6G(6795), /* Channel 169 */
        CHAN6G(6815), /* Channel 173 */
        CHAN6G(6835), /* Channel 177 */
        CHAN6G(6855), /* Channel 181 */
        CHAN6G(6875), /* Channel 185 */
        CHAN6G(6895), /* Channel 189 */
        CHAN6G(6915), /* Channel 193 */
        CHAN6G(6935), /* Channel 197 */
        CHAN6G(6955), /* Channel 201 */
        CHAN6G(6975), /* Channel 205 */
        CHAN6G(6995), /* Channel 209 */
        CHAN6G(7015), /* Channel 213 */
        CHAN6G(7035), /* Channel 217 */
        CHAN6G(7055), /* Channel 221 */
        CHAN6G(7075), /* Channel 225 */
        CHAN6G(7095), /* Channel 229 */
        CHAN6G(7115), /* Channel 233 */
};

struct wifi_hwsim_link_data {
	u32 link_id;
	u64 beacon_int	/* beacon interval in us */;
	struct hrtimer beacon_timer;
};

struct wifi_hwsim_data {
    struct list_head list;
    struct rhash_head rht;
    struct ieee80211_hw *hw;
    struct device *dev;
    struct ieee80211_supported_band bands[NUM_NL80211_BANDS];
    struct ieee80211_channel channels_2ghz[ARRAY_SIZE(hwsim_channels_2ghz)];
    struct ieee80211_channel channels_5ghz[ARRAY_SIZE(hwsim_channels_5ghz)];
    struct ieee80211_channel channels_6ghz[ARRAY_SIZE(hwsim_channels_6ghz)];
    struct ieee80211_channel channels_s1g[ARRAY_SIZE(hwsim_channels_s1g)];
    struct ieee80211_rate rates[ARRAY_SIZE(hwsim_rates)];
    struct ieee80211_iface_combination if_combination;
    struct ieee80211_iface_limit if_limits[3];
    int n_if_limits;

    u32 ciphers[ARRAY_SIZE(hwsim_ciphers)];

    struct mac_address addresses[2];
    struct ieee80211_chanctx_conf *chanctx;
    int channels, idx;
    bool use_chanctx;
    bool destroy_on_close;
    u32 portid;
    char alpha2[2];
    const struct ieee80211_regdomain *regd;

    struct ieee80211_channel *tmp_chan;
    struct ieee80211_channel *roc_chan;
    u32 roc_duration;
    struct delayed_work roc_start;
    struct delayed_work roc_done;
    struct delayed_work hw_scan;
    struct cfg80211_scan_request *hw_scan_request;
    struct ieee80211_vif *hw_scan_vif;
    int scan_chan_idx;
    u8 scan_addr[ETH_ALEN];
    struct {
        struct ieee80211_channel *channel;
        unsigned long next_start, start, end;
    } survey_data[ARRAY_SIZE(hwsim_channels_2ghz) +
                  ARRAY_SIZE(hwsim_channels_5ghz) +
                  ARRAY_SIZE(hwsim_channels_6ghz)];

    struct ieee80211_channel *channel;
    enum nl80211_chan_width bw;
    u64 beacon_int	/* beacon interval in us */;
    unsigned int rx_filter;
    bool started, idle, scanning;
    struct mutex mutex;
    struct hrtimer beacon_timer;
    enum ps_mode {
        PS_DISABLED, PS_ENABLED, PS_AUTO_POLL, PS_MANUAL_POLL
    } ps;
    bool ps_poll_pending;
    struct dentry *debugfs;

    atomic_t pending_cookie;
    struct sk_buff_head pending;	/* packets pending */
    /*
	 * Only radios in the same group can communicate together (the
	 * channel has to match too). Each bit represents a group. A
	 * radio can be in more than one group.
	 */
    u64 group;

    /* group shared by radios created in the same netns */
    int netgroup;
    /* wmediumd portid responsible for netgroup of this radio */
    u32 wmediumd;

    /* difference between this hw's clock and the real clock, in usecs */
    s64 tsf_offset;
    s64 bcn_delta;
    /* absolute beacon transmission time. Used to cover up "tx" delay. */
    u64 abs_bcn_ts;

    /* Stats */
    u64 tx_pkts;
    u64 rx_pkts;
    u64 tx_bytes;
    u64 rx_bytes;
    u64 tx_dropped;
    u64 tx_failed;

/* RSSI in rx status of the receiver */
	int rx_rssi;

	/* only used when pmsr capability is supplied */
	struct cfg80211_pmsr_capabilities pmsr_capa;
	struct cfg80211_pmsr_request *pmsr_request;
	struct wireless_dev *pmsr_request_wdev;

	struct wifi_hwsim_link_data link_data[15];
};

/**
 * enum hwsim_rate_info -- bitrate information.
 *
 * Information about a receiving or transmitting bitrate
 * that can be mapped to struct rate_info
 *
 * @HWSIM_RATE_INFO_ATTR_FLAGS: bitflag of flags from &enum rate_info_flags
 * @HWSIM_RATE_INFO_ATTR_MCS: mcs index if struct describes an HT/VHT/HE rate
 * @HWSIM_RATE_INFO_ATTR_LEGACY: bitrate in 100kbit/s for 802.11abg
 * @HWSIM_RATE_INFO_ATTR_NSS: number of streams (VHT & HE only)
 * @HWSIM_RATE_INFO_ATTR_BW: bandwidth (from &enum rate_info_bw)
 * @HWSIM_RATE_INFO_ATTR_HE_GI: HE guard interval (from &enum nl80211_he_gi)
 * @HWSIM_RATE_INFO_ATTR_HE_DCM: HE DCM value
 * @HWSIM_RATE_INFO_ATTR_HE_RU_ALLOC:  HE RU allocation (from &enum nl80211_he_ru_alloc,
 *	only valid if bw is %RATE_INFO_BW_HE_RU)
 * @HWSIM_RATE_INFO_ATTR_N_BOUNDED_CH: In case of EDMG the number of bonded channels (1-4)
 * @HWSIM_RATE_INFO_ATTR_EHT_GI: EHT guard interval (from &enum nl80211_eht_gi)
 * @HWSIM_RATE_INFO_ATTR_EHT_RU_ALLOC: EHT RU allocation (from &enum nl80211_eht_ru_alloc,
 *	only valid if bw is %RATE_INFO_BW_EHT_RU)
 * @NUM_HWSIM_RATE_INFO_ATTRS: internal
 * @HWSIM_RATE_INFO_ATTR_MAX: highest attribute number
 */
enum hwsim_rate_info_attributes {
	__HWSIM_RATE_INFO_ATTR_INVALID,

	HWSIM_RATE_INFO_ATTR_FLAGS,
	HWSIM_RATE_INFO_ATTR_MCS,
	HWSIM_RATE_INFO_ATTR_LEGACY,
	HWSIM_RATE_INFO_ATTR_NSS,
	HWSIM_RATE_INFO_ATTR_BW,
	HWSIM_RATE_INFO_ATTR_HE_GI,
	HWSIM_RATE_INFO_ATTR_HE_DCM,
	HWSIM_RATE_INFO_ATTR_HE_RU_ALLOC,
	HWSIM_RATE_INFO_ATTR_N_BOUNDED_CH,
	HWSIM_RATE_INFO_ATTR_EHT_GI,
	HWSIM_RATE_INFO_ATTR_EHT_RU_ALLOC,

	/* keep last */
	NUM_HWSIM_RATE_INFO_ATTRS,
	HWSIM_RATE_INFO_ATTR_MAX = NUM_HWSIM_RATE_INFO_ATTRS - 1
};

#endif /* APRF_DRV_H */