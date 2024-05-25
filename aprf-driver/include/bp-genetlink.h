#ifndef BP_GENETLINK_H
#define BP_GENETLINK_H
#include <net/genetlink.h>
#include <linux/version.h>

static inline void __bp_genl_info_userhdr_set(struct genl_info *info,
					      void *userhdr)
{
	info->userhdr = userhdr;
}

static inline void *__bp_genl_info_userhdr(struct genl_info *info)
{
	return info->userhdr;
}

/* this is for patches we apply */
static inline struct netlink_ext_ack *genl_info_extack(struct genl_info *info)
{
	return info->extack;
}

/* this is for patches we apply */
static inline struct netlink_ext_ack *genl_callback_extack(struct netlink_callback *cb)
{
	return cb->extack;
}

/* this gets put in place of info->userhdr, since we use that above */
static inline void *genl_info_userhdr(struct genl_info *info)
{
	return (u8 *)info->genlhdr + GENL_HDRLEN;
}

#define __genl_ro_after_init __ro_after_init

#endif /* BP_GENETLINK_H */
