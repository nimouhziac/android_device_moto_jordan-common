#ifndef __BACKPORT_NET_GENETLINK_H
#define __BACKPORT_NET_GENETLINK_H
#include_next <net/genetlink.h>

/* this is for patches we apply */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
#define genl_info_snd_portid(__genl_info) (__genl_info->snd_pid)
#else
#define genl_info_snd_portid(__genl_info) (__genl_info->snd_portid)
#endif

#ifndef GENLMSG_DEFAULT_SIZE
#define GENLMSG_DEFAULT_SIZE (NLMSG_DEFAULT_SIZE - GENL_HDRLEN)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
struct backport_genl_info {
	struct genl_info *info;

	u32 snd_seq;
	u32 snd_pid;
	struct genlmsghdr *genlhdr;
	struct nlattr **attrs;
	void *user_ptr[2];
};
#define genl_info LINUX_BACKPORT(genl_info)

/* if info gets overridden, so will family below */
#define genlmsg_put_reply(_skb, _info, _fam, _flags, _cmd)		\
	genlmsg_put_reply(_skb, (_info)->info, &(_fam)->family, _flags, _cmd)

struct backport_genl_ops {
	struct genl_ops ops;

	u8 cmd;
	u8 internal_flags;
	unsigned int flags;
	const struct nla_policy *policy;

	int (*doit)(struct sk_buff *skb, struct genl_info *info);
	int (*dumpit)(struct sk_buff *skb, struct netlink_callback *cb);
	int (*done)(struct netlink_callback *cb);
};
#define genl_ops LINUX_BACKPORT(genl_ops)

#define genlmsg_reply(_msg, _info) genlmsg_reply(_msg, (_info)->info)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32))
#define genl_info_net(_info) genl_info_net((_info)->info)
#endif
#endif /* 2.6.37 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
/*
 * struct genl_multicast_group was made netns aware through
 * patch "genetlink: make netns aware" by johannes, we just
 * force this to always use the default init_net
 */
#define genl_info_net(x) &init_net
/* Just use init_net for older kernels */
#define get_net_ns_by_pid(x) &init_net
/* net namespace is lost */
#define genlmsg_unicast(net, skb, pid) genlmsg_unicast(skb, pid)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
#define genl_dump_check_consistent(cb, user_hdr, family)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)
static inline int __real_genl_register_family(struct genl_family *family)
{
	return genl_register_family(family);
}

/* Needed for the mcgrps pointer */
struct backport_genl_family {
	struct genl_family family;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	struct list_head list;
#endif

	unsigned int id, hdrsize, version, maxattr;
	char name[GENL_NAMSIZ];
	bool netnsok;
	bool parallel_ops;

	struct nlattr **attrbuf;

	int (*pre_doit)(struct genl_ops *ops, struct sk_buff *skb,
			struct genl_info *info);

	void (*post_doit)(struct genl_ops *ops, struct sk_buff *skb,
			  struct genl_info *info);

	struct genl_multicast_group *mcgrps;
	struct genl_ops *ops;
	unsigned int n_mcgrps, n_ops;

	struct module *module;
};
#define genl_family LINUX_BACKPORT(genl_family)

int __backport_genl_register_family(struct genl_family *family);

#define genl_register_family LINUX_BACKPORT(genl_register_family)
static inline int
genl_register_family(struct genl_family *family)
{
	family->module = THIS_MODULE;
	return __backport_genl_register_family(family);
}

#define _genl_register_family_with_ops_grps \
	_backport_genl_register_family_with_ops_grps
static inline int
_genl_register_family_with_ops_grps(struct genl_family *family,
				    struct genl_ops *ops, size_t n_ops,
				    struct genl_multicast_group *mcgrps,
				    size_t n_mcgrps)
{
	family->ops = ops;
	family->n_ops = n_ops;
	family->mcgrps = mcgrps;
	family->n_mcgrps = n_mcgrps;
	return genl_register_family(family);
}

#define genl_register_family_with_ops(family, ops)			\
	_genl_register_family_with_ops_grps((family),			\
					    (ops), ARRAY_SIZE(ops),	\
					    NULL, 0)
#define genl_register_family_with_ops_groups(family, ops, grps)		\
	_genl_register_family_with_ops_grps((family),			\
					    (ops), ARRAY_SIZE(ops),	\
					    (grps), ARRAY_SIZE(grps))

#define genl_unregister_family backport_genl_unregister_family
int genl_unregister_family(struct genl_family *family);

#define genl_notify(_fam, _skb, _net, _portid, _group, _nlh, _flags)	\
	genl_notify(_skb, _net, _portid, (_fam)->mcgrps[_group].id,	\
		    _nlh, _flags)
#define genlmsg_put(_skb, _pid, _seq, _fam, _flags, _cmd)		\
	genlmsg_put(_skb, _pid, _seq, &(_fam)->family, _flags, _cmd)
#define genlmsg_nlhdr(_hdr, _fam)					\
	genlmsg_nlhdr(_hdr, &(_fam)->family)
#ifndef genl_dump_check_consistent
#define genl_dump_check_consistent(_cb, _hdr, _fam)			\
	genl_dump_check_consistent(_cb, _hdr, &(_fam)->family)
#endif
#ifndef genlmsg_put_reply /* might already be there from _info override above */
#define genlmsg_put_reply(_skb, _info, _fam, _flags, _cmd)		\
	genlmsg_put_reply(_skb, _info, &(_fam)->family, _flags, _cmd)
#endif
#define genlmsg_multicast_netns LINUX_BACKPORT(genlmsg_multicast_netns)
static inline int genlmsg_multicast_netns(struct genl_family *family,
					  struct net *net, struct sk_buff *skb,
					  u32 portid, unsigned int group,
					  gfp_t flags)
{
	if (WARN_ON_ONCE(group >= family->n_mcgrps))
		return -EINVAL;
	group = family->mcgrps[group].id;
	return nlmsg_multicast(
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
		genl_sock,
#else
		net->genl_sock,
#endif
		skb, portid, group, flags);
}
#define genlmsg_multicast LINUX_BACKPORT(genlmsg_multicast)
static inline int genlmsg_multicast(struct genl_family *family,
				    struct sk_buff *skb, u32 portid,
				    unsigned int group, gfp_t flags)
{
	if (WARN_ON_ONCE(group >= family->n_mcgrps))
		return -EINVAL;
	group = family->mcgrps[group].id;
	return nlmsg_multicast(
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
		genl_sock,
#else
		init_net.genl_sock,
#endif
		skb, portid, group, flags);
}
static inline int
backport_genlmsg_multicast_allns(struct genl_family *family,
				 struct sk_buff *skb, u32 portid,
				 unsigned int group, gfp_t flags)
{
	if (WARN_ON_ONCE(group >= family->n_mcgrps))
		return -EINVAL;
	group = family->mcgrps[group].id;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	return nlmsg_multicast(genl_sock, skb, portid, group, flags);
#else
	return genlmsg_multicast_allns(skb, portid, group, flags);
#endif
}
#define genlmsg_multicast_allns LINUX_BACKPORT(genlmsg_multicast_allns)

#define __genl_const
#else /* < 3.13 */
#define __genl_const const
#endif /* < 3.13 */

#endif /* __BACKPORT_NET_GENETLINK_H */