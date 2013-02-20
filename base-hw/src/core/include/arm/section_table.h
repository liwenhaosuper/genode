/*
 * \brief   Driver for ARM section tables
 * \author  Martin Stein
 * \date    2012-02-22
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__ARM__SECTION_TABLE_H_
#define _INCLUDE__ARM__SECTION_TABLE_H_

/* Genode includes */
#include <util/register.h>
#include <base/printf.h>

namespace Arm
{
	using namespace Genode;

	/**
	 * Map app-specific mem attributes to a TLB-specific POD
	 */
	struct Page_flags : Register<8>
	{
		struct W : Bitfield<0, 1> { }; /* writeable */
		struct X : Bitfield<1, 1> { }; /* executable */
		struct K : Bitfield<2, 1> { }; /* privileged */
		struct G : Bitfield<3, 1> { }; /* global */
		struct D : Bitfield<4, 1> { }; /* device */
		struct C : Bitfield<5, 1> { }; /* cacheable */

		/**
		 * Create flag POD for Genode pagers
		 */
		static access_t
		resolve_and_wait_for_fault(bool const writeable,
		                           bool const write_combined,
		                           bool const io_mem) {
			return W::bits(writeable) | X::bits(1) | K::bits(0) | G::bits(0) |
			       D::bits(io_mem) | C::bits(!write_combined & !io_mem); }

		/**
		 * Create flag POD for kernel when it creates the core space
		 */
		static access_t map_core_area(bool const io_mem) {
			return W::bits(1) | X::bits(1) | K::bits(0) | G::bits(0) |
			       D::bits(io_mem) | C::bits(!io_mem); }

		/**
		 * Create flag POD for the mode transition region
		 */
		static access_t mode_transition() {
			return W::bits(1) | X::bits(1) | K::bits(1) | G::bits(1) |
			       D::bits(0) | C::bits(1); }
	};

	typedef Page_flags::access_t page_flags_t;

	/**
	 * Check if 'p' is aligned to 1 << 'alignm_log2'
	 */
	inline bool aligned(addr_t const a, unsigned long const alignm_log2)
	{
		return a == ((a >> alignm_log2) << alignm_log2);
	}

	/**
	 * Common access permission [1:0] bitfield values
	 */
	struct Ap_1_0_bitfield
	{
		enum {
			KERNEL_AND_USER_NO_ACCESS = 0,
			USER_NO_ACCESS = 1,
			USER_RO_ACCESS = 2,
			KERNEL_AND_USER_SAME_ACCESS = 3
		};
	};

	/**
	 * Common access permission [2] bitfield values
	 */
	struct Ap_2_bitfield
	{
		enum {
			KERNEL_RW_OR_NO_ACCESS = 0,
			KERNEL_RO_ACCESS = 1
		};
	};

	/**
	 * Permission configuration according to given access rights
	 *
	 * \param  T  targeted translation-table-descriptor type
	 * \param  w  see 'Section_table::insert_translation'
	 * \param  x  see 'Section_table::insert_translation'
	 * \param  k  see 'Section_table::insert_translation'
	 *
	 * \return  descriptor value with requested perms and the rest left zero
	 */
	template <typename T>
	static typename T::access_t
	access_permission_bits(page_flags_t const flags)
	{
		/* lookup table for AP bitfield values according to 'w' and 'k' flag */
		typedef typename T::Ap_1_0 Ap_1_0;
		typedef typename T::Ap_2 Ap_2;
		static typename T::access_t const ap_bits[2][2] = {{
			Ap_1_0::bits(Ap_1_0::USER_RO_ACCESS) |              /* -- */
			Ap_2::bits(Ap_2::KERNEL_RW_OR_NO_ACCESS),

			Ap_1_0::bits(Ap_1_0::USER_NO_ACCESS) |              /* -k */
			Ap_2::bits(Ap_2::KERNEL_RO_ACCESS) }, {

			Ap_1_0::bits(Ap_1_0::KERNEL_AND_USER_SAME_ACCESS) | /* w- */
			Ap_2::bits(Ap_2::KERNEL_RW_OR_NO_ACCESS),

			Ap_1_0::bits(Ap_1_0::USER_NO_ACCESS) |              /* wk */
			Ap_2::bits(Ap_2::KERNEL_RW_OR_NO_ACCESS) }
		};
		/* combine XN and AP bitfield values according to the flags */
		typedef typename T::Xn Xn;
		return Xn::bits(!Page_flags::X::get(flags)) |
		       ap_bits[Page_flags::W::get(flags)][Page_flags::K::get(flags)];
	}

	/**
	 * Wether support for caching is already enabled
	 *
	 * FIXME: Normally all ARM platforms should support caching,
	 *        but for some 'base_hw' misses support by now.
	 */
	inline bool cache_support();

	/**
	 * Memory region attributes for the translation descriptor 'T'
	 */
	template <typename T>
	static typename T::access_t memory_region_attr(page_flags_t const flags)
	{
		typedef typename T::Tex Tex;
		typedef typename T::C C;
		typedef typename T::B B;

		/*
		 * FIXME: upgrade to write-back & write-allocate when !d & c
		 */
		if(Page_flags::D::get(flags))
			    return Tex::bits(2) | C::bits(0) | B::bits(0);
		if(cache_support()) {
			if(Page_flags::C::get(flags))
				return Tex::bits(6) | C::bits(1) | B::bits(0);
			    return Tex::bits(4) | C::bits(0) | B::bits(0);
		}
		        return Tex::bits(4) | C::bits(0) | B::bits(0);
	}

	/**
	 * Second level translation table
	 *
	 * A table is dedicated to either secure or non-secure mode. All
	 * translations done by this table apply to domain 0. They are not
	 * shareable and have zero-filled memory region attributes.
	 */
	class Page_table
	{
		enum {
			_1KB_LOG2 = 10,
			_4KB_LOG2 = 12,
			_64KB_LOG2 = 16,
			_1MB_LOG2 = 20,
		};

		public:

			enum {
				SIZE_LOG2 = _1KB_LOG2,
				SIZE = 1 << SIZE_LOG2,
				ALIGNM_LOG2 = SIZE_LOG2,

				VIRT_SIZE_LOG2 = _1MB_LOG2,
				VIRT_SIZE = 1 << VIRT_SIZE_LOG2,
				VIRT_BASE_MASK = ~((1 << VIRT_SIZE_LOG2) - 1),
			};

		protected:

			/**
			 * Common descriptor structure
			 */
			struct Descriptor : Register<32>
			{
				/**
				 * Descriptor types
				 */
				enum Type { FAULT, SMALL_PAGE, LARGE_PAGE };

				struct Type_1 : Bitfield<1, 1> { };
				struct Type_2 : Bitfield<0, 1> { };

				/**
				 * Get descriptor type of 'v'
				 */
				static Type type(access_t const v)
				{
					access_t const t1 = Type_1::get(v);
					if (t1 == 0) {
						access_t const t2 = Type_2::get(v);
						if (t2 == 0) return FAULT;
						if (t2 == 1) return LARGE_PAGE;
					}
					if (t1 == 1) return SMALL_PAGE;
					return FAULT;
				}

				/**
				 * Set descriptor type of 'v'
				 */
				static void type(access_t & v, Type const t)
				{
					switch (t) {

					case FAULT: {

						Type_1::set(v, 0);
						Type_2::set(v, 0);
						break; }

					case SMALL_PAGE: {

						Type_1::set(v, 1);
						break; }

					case LARGE_PAGE: {

						Type_1::set(v, 0);
						Type_2::set(v, 1);
						break; }
					}
				}

				/**
				 * Invalidate descriptor 'v'
				 */
				static void invalidate(access_t & v) { type(v, FAULT); }

				/**
				 * Return if descriptor 'v' is valid
				 */
				static bool valid(access_t & v) { return type(v) != FAULT; }
			};

			/**
			 * Represents an untranslated virtual region
			 */
			struct Fault : Descriptor
			{
				enum {
					VIRT_SIZE_LOG2 = _4KB_LOG2,
					VIRT_SIZE = 1 << VIRT_SIZE_LOG2,
					VIRT_BASE_MASK = ~((1 << VIRT_SIZE_LOG2) - 1)
				};
			};

			/**
			 * Small page descriptor structure
			 */
			struct Small_page : Descriptor
			{
				enum {
					VIRT_SIZE_LOG2   = _4KB_LOG2,
					VIRT_SIZE        = 1 << VIRT_SIZE_LOG2,
					VIRT_OFFSET_MASK = (1 << VIRT_SIZE_LOG2) - 1,
					VIRT_BASE_MASK   = ~(VIRT_OFFSET_MASK),
				};

				struct Xn       : Bitfield<0, 1> { };   /* execute never */
				struct B        : Bitfield<2, 1> { };   /* mem region attr. */
				struct C        : Bitfield<3, 1> { };   /* mem region attr. */
				struct Ap_1_0   : Bitfield<4, 2>,       /* access permission */
				                  Ap_1_0_bitfield { };
				struct Tex      : Bitfield<6, 3> { };   /* mem region attr. */
				struct Ap_2     : Bitfield<9, 1>,       /* access permission */
				                  Ap_2_bitfield { };
				struct S        : Bitfield<10, 1> { };  /* shareable bit */
				struct Ng       : Bitfield<11, 1> { };  /* not global bit */
				struct Pa_31_12 : Bitfield<12, 20> { }; /* physical base */

				/**
				 * Compose descriptor value
				 */
				static access_t create(page_flags_t const flags,
				                       addr_t const pa)
				{
					access_t v = access_permission_bits<Small_page>(flags) |
					             memory_region_attr<Small_page>(flags) |
					             Ng::bits(!Page_flags::G::get(flags)) |
					             S::bits(0) | Pa_31_12::masked(pa);
					Descriptor::type(v, Descriptor::SMALL_PAGE);
					return v;
				}
			};

			/*
			 * Table payload
			 *
			 * Must be the only member of this class
			 */
			Descriptor::access_t _entries[SIZE/sizeof(Descriptor::access_t)];

			enum { MAX_INDEX = sizeof(_entries) / sizeof(_entries[0]) - 1 };

			/**
			 * Get entry index by virtual offset
			 *
			 * \param i   is overridden with the index if call returns 0
			 * \param vo  virtual offset relative to the virtual table base
			 *
			 * \retval  0  on success
			 * \retval <0  translation failed
			 */
			int _index_by_vo (unsigned long & i, addr_t const vo) const
			{
				if (vo > max_virt_offset()) return -1;
				i = vo >> Small_page::VIRT_SIZE_LOG2;
				return 0;
			}

		public:

			/**
			 * Placement new operator
			 */
			void * operator new (size_t, void * p) { return p; }

			/**
			 * Constructor
			 */
			Page_table()
			{
				/* check table alignment */
				if (!aligned((addr_t)this, ALIGNM_LOG2) ||
				    (addr_t)this != (addr_t)_entries)
				{
					PDBG("Insufficient table alignment");
					while (1) ;
				}
				/* start with an empty table */
				for (unsigned i = 0; i <= MAX_INDEX; i++)
					Descriptor::invalidate(_entries[i]);
			}

			/**
			 * Maximum virtual offset that can be translated by this table
			 */
			static addr_t max_virt_offset()
			{
				return (MAX_INDEX << Small_page::VIRT_SIZE_LOG2)
				       + (Small_page::VIRT_SIZE - 1);
			}

			/**
			 * Insert one atomic translation into this table
			 *
			 * \param vo         offset of the virtual region represented
			 *                   by the translation within the virtual
			 *                   region represented by this table
			 * \param pa         base of the physical backing store
			 * \param size_log2  log2(Size of the translated region),
			 *                   must be supported by this table
			 * \param flags      mapping flags
			 *
			 * This method overrides an existing translation in case
			 * that it spans the the same virtual range and is not
			 * a link to another table level.
			 */
			void insert_translation(addr_t const vo, addr_t const pa,
			                        unsigned long const size_log2,
			                        page_flags_t const flags)
			{
				/* validate virtual address */
				unsigned long i;
				if (_index_by_vo (i, vo)) {
					PDBG("Invalid virtual offset");
					while (1) ;
				}
				/* select descriptor type by the translation size */
				if (size_log2 == Small_page::VIRT_SIZE_LOG2)
				{
					/* compose new descriptor value */
					Descriptor::access_t const entry =
						Small_page::create(flags, pa);

					/* check if we can we write to the targeted entry */
					if (Descriptor::valid(_entries[i]))
					{
						/*
						 * It's possible that multiple threads fault at the
						 * same time on the same translation, thus we need
						 * this check.
						 */
						if (_entries[i] == entry) return;

						/* never modify existing translations */
						PDBG("Couldn't override entry");
						while (1) ;
					}
					/* override table entry with new descriptor value */
					_entries[i] = entry;
					return;
				}
				PDBG("Translation size not supported");
				while (1) ;
			}

			/**
			 * Remove translations, wich overlap with a given virtual region
			 *
			 * \param vo    offset of the virtual region within the region
			 *              represented by this table
			 * \param size  region size
			 */
			void remove_region(addr_t const vo, size_t const size)
			{
				/* traverse all possibly affected entries */
				addr_t residual_vo = vo;
				unsigned long i;
				while (1)
				{
					/* check if anything is left over to remove */
					if (residual_vo >= vo + size) return;

					/* check if residual region overlaps with table */
					if (_index_by_vo(i, residual_vo)) return;

					/* update current entry and recalculate residual region */
					switch (Descriptor::type(_entries[i])) {

					case Descriptor::FAULT: {

						residual_vo = (residual_vo & Fault::VIRT_BASE_MASK)
						              + Fault::VIRT_SIZE;
						break; }

					case Descriptor::SMALL_PAGE: {

						residual_vo = (residual_vo & Small_page::VIRT_BASE_MASK)
						              + Small_page::VIRT_SIZE;
						Descriptor::invalidate(_entries[i]);
						break; }

					case Descriptor::LARGE_PAGE: {

						PDBG("Removal of large pages not implemented");
						while (1) ;
						break; }
					}
				}
			}

			/**
			 * Does this table solely contain invalid entries
			 */
			bool empty()
			{
				for (unsigned i = 0; i <= MAX_INDEX; i++) {
					if (Descriptor::valid(_entries[i])) return false;
				}
				return true;
			}

			/**
			 * Get next translation size log2 by area constraints
			 *
			 * \param vo  virtual offset within this table
			 * \param s   area size
			 */
			static unsigned
			translation_size_l2(addr_t const vo, size_t const s)
			{
				off_t const o = vo & Small_page::VIRT_OFFSET_MASK;
				if (!o && s >= Small_page::VIRT_SIZE)
					return Small_page::VIRT_SIZE_LOG2;
				PDBG("Insufficient alignment or size");
				while (1) ;
			}

	} __attribute__((aligned(1<<Page_table::ALIGNM_LOG2)));

	/**
	 * First level translation table
	 *
	 * A table is dedicated to either secure or non-secure mode. All
	 * translations done by this table apply to domain 0. They are not
	 * shareable and have zero-filled memory region attributes. The size
	 * of this table is fixed to such a value that this table translates
	 * a space wich is addressable by 32 bit.
	 */
	class Section_table
	{
		enum {
			_16KB_LOG2 = 14,
			_1MB_LOG2 = 20,
			_16MB_LOG2 = 24,

			DOMAIN = 0,
		};

		public:

			enum {
				SIZE_LOG2 = _16KB_LOG2,
				SIZE = 1 << SIZE_LOG2,
				ALIGNM_LOG2 = SIZE_LOG2,

				VIRT_SIZE_LOG2 = _1MB_LOG2,
				VIRT_SIZE = 1 << VIRT_SIZE_LOG2,
				VIRT_BASE_MASK = ~((1 << VIRT_SIZE_LOG2) - 1),

				MAX_COSTS_PER_TRANSLATION = sizeof(Page_table),

				MAX_PAGE_SIZE_LOG2 = 20,
				MIN_PAGE_SIZE_LOG2 = 12,
			};

			/**
			 * A first level translation descriptor
			 */
			struct Descriptor : Register<32>
			{
				/**
				 * Descriptor types
				 */
				enum Type { FAULT, PAGE_TABLE, SECTION, SUPERSECTION };

				struct Type_1 : Bitfield<0, 2> { };  /* entry type code 1 */
				struct Type_2 : Bitfield<18, 1> { }; /* entry type code 2 */

				/**
				 * Get descriptor type of 'v'
				 */
				static Type type(access_t const v)
				{
					access_t const t1 = Type_1::get(v);
					if (t1 == 0) return FAULT;
					if (t1 == 1) return PAGE_TABLE;
					if (t1 == 2) {
						access_t const t2 = Type_2::get(v);
						if (t2 == 0) return SECTION;
						if (t2 == 1) return SUPERSECTION;
					}
					return FAULT;
				}

				/**
				 * Set descriptor type of 'v'
				 */
				static void type(access_t & v, Type const t)
				{
					switch (t) {
					case FAULT: Type_1::set(v, 0); break;
					case PAGE_TABLE: Type_1::set(v, 1); break;
					case SECTION:
						Type_1::set(v, 2);
						Type_2::set(v, 0); break;
					case SUPERSECTION:
						Type_1::set(v, 2);
						Type_2::set(v, 1); break;
					}
				}

				/**
				 * Invalidate descriptor 'v'
				 */
				static void invalidate(access_t & v) { type(v, FAULT); }

				/**
				 * Return if descriptor 'v' is valid
				 */
				static bool valid(access_t & v) { return type(v) != FAULT; }
			};

			/**
			 * Represents an untranslated virtual region
			 */
			struct Fault : Descriptor
			{
				enum {
					VIRT_SIZE_LOG2 = _1MB_LOG2,
					VIRT_SIZE = 1 << VIRT_SIZE_LOG2,
					VIRT_BASE_MASK = ~((1 << VIRT_SIZE_LOG2) - 1)
				};
			};

			/**
			 * Link to a second level translation table
			 */
			struct Page_table_descriptor : Descriptor
			{
				struct Domain   : Bitfield<5, 4> { };   /* domain */
				struct Pa_31_10 : Bitfield<10, 22> { }; /* physical base */

				/**
				 * Compose descriptor value
				 */
				static access_t create(Page_table * const pt)
				{
						access_t v = Domain::bits(DOMAIN) |
						             Pa_31_10::masked((addr_t)pt);
						Descriptor::type(v, Descriptor::PAGE_TABLE);
						return v;
				}
			};

			/**
			 * Section translation descriptor
			 */
			struct Section : Descriptor
			{
				enum {
					VIRT_SIZE_LOG2   = _1MB_LOG2,
					VIRT_SIZE        = 1 << VIRT_SIZE_LOG2,
					VIRT_OFFSET_MASK = (1 << VIRT_SIZE_LOG2) - 1,
					VIRT_BASE_MASK   = ~(VIRT_OFFSET_MASK),
				};

				struct B        : Bitfield<2, 1> { };   /* mem. region attr. */
				struct C        : Bitfield<3, 1> { };   /* mem. region attr. */
				struct Xn       : Bitfield<4, 1> { };   /* execute never bit */
				struct Domain   : Bitfield<5, 4> { };   /* domain */
				struct Ap_1_0   : Bitfield<10, 2>,      /* access permission */
				                  Ap_1_0_bitfield { };
				struct Tex      : Bitfield<12, 3> { };  /* mem. region attr. */
				struct Ap_2     : Bitfield<15, 1>,      /* access permission */
				                  Ap_2_bitfield { };
				struct S        : Bitfield<16, 1> { };  /* shared */
				struct Ng       : Bitfield<17, 1> { };  /* not global */
				struct Pa_31_20 : Bitfield<20, 12> { }; /* physical base */

				/**
				 * Compose descriptor value
				 */
				static access_t create(page_flags_t const flags,
				                       addr_t const pa)
				{
					access_t v = access_permission_bits<Section>(flags) |
					             memory_region_attr<Section>(flags) |
					             Domain::bits(DOMAIN) | S::bits(0) |
					             Ng::bits(!Page_flags::G::get(flags)) |
					             Pa_31_20::masked(pa);
					Descriptor::type(v, Descriptor::SECTION);
					return v;
				}
			};

		protected:

			/* table payload, must be the first member of this class */
			Descriptor::access_t _entries[SIZE/sizeof(Descriptor::access_t)];

			enum { MAX_INDEX = sizeof(_entries) / sizeof(_entries[0]) - 1 };

			/**
			 * Get entry index by virtual offset
			 *
			 * \param i    is overridden with the resulting index
			 * \param vo   offset within the virtual region represented
			 *             by this table
			 *
			 * \retval  0  on success
			 * \retval <0  if virtual offset couldn't be resolved,
			 *             in this case 'i' reside invalid
			 */
			int _index_by_vo(unsigned long & i, addr_t const vo) const
			{
				if (vo > max_virt_offset()) return -1;
				i = vo >> Section::VIRT_SIZE_LOG2;
				return 0;
			}

		public:

			/**
			 * Constructor
			 */
			Section_table()
			{
				/* check for appropriate positioning of the table */
				if (!aligned((addr_t)this, ALIGNM_LOG2)
				    || (addr_t)this != (addr_t)_entries)
				{
					PDBG("Insufficient table alignment");
					while (1) ;
				}
				/* start with an empty table */
				for (unsigned i = 0; i <= MAX_INDEX; i++)
					Descriptor::invalidate(_entries[i]);
			}

			/**
			 * Maximum virtual offset that can be translated by this table
			 */
			static addr_t max_virt_offset()
			{
				return (MAX_INDEX << Section::VIRT_SIZE_LOG2)
				       + (Section::VIRT_SIZE - 1);
			}

			/**
			 * Insert one atomic translation into this table
			 *
			 * \param ST           platform specific section-table type
			 * \param st           platform specific section table
			 * \param vo           offset of the virtual region represented
			 *                     by the translation within the virtual
			 *                     region represented by this table
			 * \param pa           base of the physical backing store
			 * \param size_log2    size log2 of the translated region
			 * \param flags        mapping flags
			 * \param extra_space  If > 0, it must point to a portion of
			 *                     size-aligned memory space wich may be used
			 *                     furthermore by the table for the incurring
			 *                     administrative costs of the  translation.
			 *                     To determine the amount of additionally
			 *                     needed memory one can instrument this
			 *                     method with 'extra_space' set to 0.
			 *                     The so donated memory may be regained by
			 *                     using the method 'regain_memory'.
			 *
			 * \retval  0  translation successfully inserted
			 * \retval >0  Translation not inserted, the return value
			 *             is the size log2 of additional size-aligned
			 *             space that is needed to do the translation.
			 *             This occurs solely when 'extra_space' is 0.
			 *
			 * This method overrides an existing translation in case that it
			 * spans the the same virtual range and is not a link to another
			 * table level.
			 */
			template <typename ST>
			unsigned long insert_translation(addr_t const vo, addr_t const pa,
			                                 unsigned long const size_log2,
			                                 page_flags_t const flags,
			                                 ST * const st,
			                                 void * const extra_space = 0)
			{
				typedef typename ST::Section Section;
				typedef typename ST::Page_table_descriptor Page_table_descriptor;

				/* validate virtual address */
				unsigned long i;
				if (_index_by_vo (i, vo)) {
					PDBG("Invalid virtual offset");
					while (1) ;
				}
				/* select descriptor type by translation size */
				if (size_log2 < Section::VIRT_SIZE_LOG2)
				{
					/* check if an appropriate page table already exists */
					Page_table * pt;
					if (Descriptor::type(_entries[i]) == Descriptor::PAGE_TABLE) {
						pt = (Page_table *)(addr_t)
							Page_table_descriptor::Pa_31_10::masked(_entries[i]);
					}
					/* check if we have enough memory for the page table */
					else if (extra_space)
					{
						/* check if we can write to the targeted entry */
						if (Descriptor::valid(_entries[i])) {
							PDBG ("Couldn't override entry");
							while (1) ;
						}
						/* create and link page table */
						pt = new (extra_space) Page_table();
						_entries[i] = Page_table_descriptor::create(pt, st);
					}
					/* request additional memory to create a page table */
					else return Page_table::SIZE_LOG2;

					/* insert translation */
					pt->insert_translation(vo - Section::Pa_31_20::masked(vo),
					                       pa, size_log2, flags);
					return 0;
				}
				if (size_log2 == Section::VIRT_SIZE_LOG2)
				{
					/* compose section descriptor */
					Descriptor::access_t const entry =
						Section::create(flags, pa, st);

					/* check if we can we write to the targeted entry */
					if (Descriptor::valid(_entries[i]))
					{
						/*
						 * It's possible that multiple threads fault at the
						 * same time on the same translation, thus we need
						 * this check.
						 */
						if (_entries[i] == entry) return 0;

						/* never modify existing translations */
						PDBG("Couldn't override entry");
						while (1) ;
					}
					/* override the table entry */
					_entries[i] = entry;
					return 0;
				}
				PDBG("Translation size not supported");
				while (1) ;
			}

			/**
			 * Remove translations, wich overlap with a given virtual region
			 *
			 * \param vo    offset of the virtual region within the region
			 *              represented by this table
			 * \param size  region size
			 */
			void remove_region(addr_t const vo, size_t const size)
			{
				/* traverse all possibly affected entries */
				addr_t residual_vo = vo;
				unsigned long i;
				while (1)
				{
					/* check if anything is left over to remove */
					if (residual_vo >= vo + size) return;

					/* check if the residual region overlaps with this table */
					if (_index_by_vo(i, residual_vo)) return;

					/* update current entry and recalculate residual region */
					switch (Descriptor::type(_entries[i]))
					{
					case Descriptor::FAULT:
					{
						residual_vo = (residual_vo & Fault::VIRT_BASE_MASK)
						              + Fault::VIRT_SIZE;
						break;
					}
					case Descriptor::PAGE_TABLE:
					{
						/* instruct page table to remove residual region */
						Page_table * const pt = (Page_table *)
						(addr_t)Page_table_descriptor::Pa_31_10::masked(_entries[i]);
						size_t const residual_size = vo + size - residual_vo;
						addr_t const pt_vo = residual_vo
						                     - Section::Pa_31_20::masked(residual_vo);
						pt->remove_region(pt_vo, residual_size);

						/* recalculate residual region */
						residual_vo = (residual_vo & Page_table::VIRT_BASE_MASK)
						              + Page_table::VIRT_SIZE;
						break;
					}
					case Descriptor::SECTION:
					{
						Descriptor::invalidate(_entries[i]);
						residual_vo = (residual_vo & Section::VIRT_BASE_MASK)
						              + Section::VIRT_SIZE;
						break;
					}
					case Descriptor::SUPERSECTION:
					{
						PDBG("Removal of supersections not implemented");
						while (1);
						break;
					}
					}
				}
			}

			/**
			 * Get base address for hardware table walk
			 */
			addr_t base() const { return (addr_t)_entries; }

			/**
			 * Get a portion of memory that is no longer used by this table
			 *
			 * \param base  base of regained mem portion if method returns 1
			 * \param s     size of regained mem portion if method returns 1
			 *
			 * \retval 1  successfully regained memory
			 * \retval 0  no more memory to regain
			 */
			bool regain_memory (void * & base, size_t & s)
			{
				/* walk through all entries */
				for (unsigned i = 0; i <= MAX_INDEX; i++)
				{
					if (Descriptor::type(_entries[i]) == Descriptor::PAGE_TABLE)
					{
						Page_table * const pt = (Page_table *)
						(addr_t)Page_table_descriptor::Pa_31_10::masked(_entries[i]);
						if (pt->empty())
						{
							/* we've found an useless page table */
							Descriptor::invalidate(_entries[i]);
							base = (void *)pt;
							s = sizeof(Page_table);
							return true;
						}
					}
				}
				return false;
			}

			/**
			 * Get next translation size log2 by area constraints
			 *
			 * \param vo  virtual offset within this table
			 * \param s   area size
			 */
			static unsigned
			translation_size_l2(addr_t const vo, size_t const s)
			{
				off_t const o = vo & Section::VIRT_OFFSET_MASK;
				if (!o && s >= Section::VIRT_SIZE)
					return Section::VIRT_SIZE_LOG2;
				return Page_table::translation_size_l2(o, s);
			}

			/**
			 * Insert translations for given area, do not permit displacement
			 *
			 * \param vo     virtual offset within this table
			 * \param s      area size
			 * \param flags  mapping flags
			 */
			template <typename ST>
			void map_core_area(addr_t vo, size_t s, bool io_mem, ST * st)
			{
				/* initialize parameters */
				page_flags_t const flags = Page_flags::map_core_area(io_mem);
				unsigned tsl2 = translation_size_l2(vo, s);
				size_t ts = 1 << tsl2;

				/* walk through the area and map all offsets */
				while (1)
				{
					/* map current offset without displacement */
					if(st->insert_translation(vo, vo, tsl2, flags)) {
						PDBG("Displacement not permitted");
						return;
					}
					/* update parameters for next round or exit */
					vo += ts;
					s = ts < s ? s - ts : 0;
					if (!s) return;
					tsl2 = translation_size_l2(vo, s);
					ts   = 1 << tsl2;
				}
			}

	} __attribute__((aligned(1<<Section_table::ALIGNM_LOG2)));
}

#endif /* _INCLUDE__ARM__SECTION_TABLE_H_ */

