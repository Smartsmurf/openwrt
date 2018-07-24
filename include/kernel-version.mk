# Use the default kernel version if the Makefile doesn't override it

LINUX_RELEASE?=1

LINUX_VERSION-3.18 = .71
LINUX_VERSION-4.4 = .121
LINUX_VERSION-4.9 = .91
LINUX_VERSION-4.14 = .32
LINUX_VERSION-4.16 = .18
LINUX_VERSION-4.17 = .9

LINUX_KERNEL_HASH-3.18.71 = 5abc9778ad44ce02ed6c8ab52ece8a21c6d20d21f6ed8a19287b4a38a50c1240
LINUX_KERNEL_HASH-4.4.121 = 44a88268b5088dc326b30c9b9133ac35a9a200b636b7268d08f32abeae6ca729
LINUX_KERNEL_HASH-4.9.91 = 60caa752ec9fa1c426f6a2f37db3f268d0961b67a723b6443949112167b39832
LINUX_KERNEL_HASH-4.14.32 = cb0979bec663089a43b10cfbeae0cf9673544b0ff5968c33ede614ec0f43b680
LINUX_KERNEL_HASH-4.16.2 = 470b1fe3b8ee5d1e8e0be5c4e5928b6d5bc00e9ab6c4cff18ff680d3ef20f894
LINUX_KERNEL_HASH-4.16.3 = 0d6971a81da97e38b974c5eba31a74803bfe41aabc46d406c3acda56306c81a3
LINUX_KERNEL_HASH-4.16.9 = 6037bb398ee6ae1a3d917cb575b61e7741fa084ba397e08abc271bf09d6d9b8a
LINUX_KERNEL_HASH-4.16.15 = 58f2a6732d240a45322495d4839a80c8049eac7caa1f4f44860565665579236c
LINUX_KERNEL_HASH-4.16.18 = 41026d713ba4f7a5e9d514b876ce4ed28a1d993c0c58b42b2a2597d6a0e83021
LINUX_KERNEL_HASH-4.17.9 = fd14dcadd37860bd35e1ffe7380d8ff0fb438135a95771021722914871783abb

remove_uri_prefix=$(subst git://,,$(subst http://,,$(subst https://,,$(1))))
sanitize_uri=$(call qstrip,$(subst @,_,$(subst :,_,$(subst .,_,$(subst -,_,$(subst /,_,$(1)))))))

ifneq ($(call qstrip,$(CONFIG_KERNEL_GIT_CLONE_URI)),)
  LINUX_VERSION:=$(call sanitize_uri,$(call remove_uri_prefix,$(CONFIG_KERNEL_GIT_CLONE_URI)))
  ifeq ($(call qstrip,$(CONFIG_KERNEL_GIT_REF)),)
    CONFIG_KERNEL_GIT_REF:=HEAD
  endif
  LINUX_VERSION:=$(LINUX_VERSION)-$(call sanitize_uri,$(CONFIG_KERNEL_GIT_REF))
else
ifdef KERNEL_PATCHVER
  LINUX_VERSION:=$(KERNEL_PATCHVER)$(strip $(LINUX_VERSION-$(KERNEL_PATCHVER)))
endif
endif

split_version=$(subst ., ,$(1))
merge_version=$(subst $(space),.,$(1))
KERNEL_BASE=$(firstword $(subst -, ,$(LINUX_VERSION)))
KERNEL=$(call merge_version,$(wordlist 1,2,$(call split_version,$(KERNEL_BASE))))
KERNEL_PATCHVER ?= $(KERNEL)

# disable the md5sum check for unknown kernel versions
LINUX_KERNEL_HASH:=$(LINUX_KERNEL_HASH-$(strip $(LINUX_VERSION)))
LINUX_KERNEL_HASH?=x
