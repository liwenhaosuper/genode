/*
 * \brief   Kernel representation of a user thread
 * \author  Martin Stein
 * \date    2012-11-30
 */

/*
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CORE__KERNEL__THREAD_H_
#define _CORE__KERNEL__THREAD_H_

/* Genode includes */
#include <util/fifo.h>
#include <util/avl_tree.h>

/* core includes */
#include <kernel_support.h>
#include <assert.h>

namespace Genode
{
	class Platform_thread;
}

namespace Kernel
{
	typedef Genode::addr_t          addr_t;
	typedef Genode::size_t          size_t;
	typedef Genode::Signal          Signal;
	typedef Genode::Pagefault       Pagefault;
	typedef Genode::Native_utcb     Native_utcb;
	typedef Genode::Platform_thread Platform_thread;

	template <typename T> class Avl_tree : public Genode::Avl_tree<T> { };
	template <typename T> class Avl_node : public Genode::Avl_node<T> { };
	template <typename T> class Fifo     : public Genode::Fifo<T> { };

	/* kernel configuration */
	enum {
		DEFAULT_STACK_SIZE = 1*1024*1024,
		USER_TIME_SLICE_MS = 10,
		MAX_PDS = 256,
		MAX_THREADS = 256,
		MAX_SIGNAL_RECEIVERS = 256,
		MAX_SIGNAL_CONTEXTS = 256,
		MAX_VMS = 4,
	};

	/**
	 * Map unique sortable IDs to object pointers
	 *
	 * \param OBJECT_T  object type that should be inherited
	 *                  from 'Object_pool::Entry'
	 */
	template <typename _OBJECT_T>
	class Object_pool
	{
		typedef _OBJECT_T Object;

		public:

			enum { INVALID_ID = 0 };

			/**
			 * Provide 'Object_pool'-entry compliance by inheritance
			 */
			class Entry : public Avl_node<Entry>
			{
				protected:

					unsigned long _id;

				public:

					/**
					 * Constructors
					 */
					Entry(unsigned long const id) : _id(id) { }

					/**
					 * Find entry with 'object_id' within this AVL subtree
					 */
					Entry * find(unsigned long const object_id)
					{
						if (object_id == id()) return this;
						Entry * const subtree = Avl_node<Entry>::child(object_id > id());
						return subtree ? subtree->find(object_id) : 0;
					}

					/**
					 * ID of this object
					 */
					unsigned long const id() const { return _id; }


					/************************
					 * 'Avl_node' interface *
					 ************************/

					bool higher(Entry *e) { return e->id() > id(); }
			};

		private:

			Avl_tree<Entry> _tree;

		public:

			/**
			 * Add 'object' to pool
			 */
			void insert(Object * const object) { _tree.insert(object); }

			/**
			 * Remove 'object' from pool
			 */
			void remove(Object * const object) { _tree.remove(object); }

			/**
			 * Lookup object
			 */
			Object * object(unsigned long const id)
			{
				Entry * object = _tree.first();
				return (Object *)(object ? object->find(id) : 0);
			}
	};

	/**
	 * Manage allocation of a static set of IDs
	 *
	 * \param _SIZE  How much IDs shall be assignable simultaneously
	 */
	template <unsigned _SIZE>
	class Id_allocator
	{
		enum { MIN = 1, MAX = _SIZE };

		bool _free[MAX + 1]; /* assignability bitmap */
		unsigned _first_free_id; /* hint to optimze access */

		/**
		 * Update first free ID after assignment
		 */
		void _first_free_id_assigned()
		{
			_first_free_id++;
			while (_first_free_id <= MAX) {
				if (_free[_first_free_id]) break;
				_first_free_id++;
			}
		}

		/**
		 * Validate ID
		 */
		bool _valid_id(unsigned const id) const
		{ return id >= MIN && id <= MAX; }

		public:

			/**
			 * Constructor, makes all IDs unassigned
			 */
			Id_allocator() : _first_free_id(MIN)
			{ for (unsigned i = MIN; i <= MAX; i++) _free[i] = 1; }

			/**
			 * Allocate an unassigned ID
			 *
			 * \return  ID that has been allocated by the call
			 */
			unsigned alloc()
			{
				if (!_valid_id(_first_free_id)) assert(0);
				_free[_first_free_id] = 0;
				unsigned const id = _first_free_id;
				_first_free_id_assigned();
				return id;
			}

			/**
			 * Free a given ID
			 */
			void free(unsigned const id)
			{
				if (!_valid_id(id)) return;
				_free[id] = 1;
				if (id < _first_free_id) _first_free_id = id;
			}
	};

	/**
	 * Provides kernel object management for 'T'-objects if 'T' derives from it
	 */
	template <typename T, unsigned MAX_INSTANCES>
	class Object : public Object_pool<T>::Entry
	{
		typedef Id_allocator<MAX_INSTANCES> Id_alloc;

		/**
		 * Allocator for unique IDs for all instances of 'T'
		 */
		static Id_alloc * _id_alloc()
		{
			static Id_alloc _id_alloc;
			return &_id_alloc;
		}

		public:

			typedef Object_pool<T> Pool;

			/**
			 * Gets every instance of 'T' by its ID
			 */
			static Pool * pool()
			{
				static Pool _pool;
				return &_pool;
			}

			/**
			 * Placement new
			 *
			 * Kernel objects are normally constructed on a memory
			 * donation so we must be enabled to place them explicitly.
			 */
			void * operator new (size_t, void * p) { return p; }

		protected:

			/**
			 * Constructor
			 *
			 * Ensures that we have a unique ID and
			 * can be found through the static object pool.
			 */
			Object() : Pool::Entry(_id_alloc()->alloc()) {
				pool()->insert(static_cast<T *>(this)); }

			/**
			 * Destructor
			 */
			~Object()
			{
				pool()->remove(static_cast<T *>(this));
				_id_alloc()->free(Pool::Entry::id());
			}
	};

	/**
	 * Double connected list
	 *
	 * \param _ENTRY_T  list entry type
	 */
	template <typename _ENTRY_T>
	class Double_list
	{
		private:

			_ENTRY_T * _head;
			_ENTRY_T * _tail;

		public:

			/**
			 * Provide 'Double_list'-entry compliance by inheritance
			 */
			class Entry
			{
				friend class Double_list<_ENTRY_T>;

				private:

					_ENTRY_T * _next;
					_ENTRY_T * _prev;
					Double_list<_ENTRY_T> * _list;

				public:

					/**
					 * Constructor
					 */
					Entry() : _next(0), _prev(0), _list(0) { }


					/***************
					 ** Accessors **
					 ***************/

					_ENTRY_T * next() const { return _next; }

					_ENTRY_T * prev() const { return _prev; }
			};

		public:

			/**
			 * Constructor
			 *
			 * Start with an empty list.
			 */
			Double_list(): _head(0), _tail(0) { }

			/**
			 * Insert entry from behind into list
			 */
			void insert_tail(_ENTRY_T * const e)
			{
				/* avoid leaking lists */
				if (e->Entry::_list)
					e->Entry::_list->remove(e);

				/* update new entry */
				e->Entry::_prev = _tail;
				e->Entry::_next = 0;
				e->Entry::_list = this;

				/* update previous entry or _head */
				if (_tail) _tail->Entry::_next = e;  /* List was not empty */
				else       _head = e;                /* List was empty */
				           _tail = e;
			}

			/**
			 * Remove specific entry from list
			 */
			void remove(_ENTRY_T * const e)
			{
				/* sanity checks */
				if (!_head || e->Entry::_list != this) return;

				/* update next entry or _tail */
				if (e != _tail) e->Entry::_next->Entry::_prev = e->Entry::_prev;
				else _tail = e->Entry::_prev;

				/* update previous entry or _head */
				if (e != _head) e->Entry::_prev->Entry::_next = e->Entry::_next;
				else _head = e->Entry::_next;

				/* update removed entry */
				e->Entry::_list = 0;
			}

			/**
			 * Remove head from list and return it
			 */
			_ENTRY_T * remove_head()
			{
				/* sanity checks */
				if (!_head) return 0;

				/* update _head */
				_ENTRY_T * const e = _head;
				_head = e->Entry::_next;

				/* update next entry or _tail */
				if (_head) _head->Entry::_prev = 0;
				else _tail = 0;

				/* update removed entry */
				e->Entry::_list = 0;
				return e;
			}

			/**
			 * Remove head from list and insert it at the end
			 */
			void head_to_tail()
			{
				/* sanity checks */
				if (!_head || _head == _tail) return;

				/* remove entry */
				_ENTRY_T * const e = _head;
				_head = _head->Entry::_next;
				e->Entry::_next = 0;
				_head->Entry::_prev = 0;

				/* insert entry */
				_tail->Entry::_next = e;
				e->Entry::_prev = _tail;
				_tail = e;
			}


			/***************
			 ** Accessors **
			 ***************/

			_ENTRY_T * head() const { return _head; }

			_ENTRY_T * tail() const { return _tail; }
	};

	/**
	 * Simple round robin scheduler for 'ENTRY_T' typed clients
	 */
	template <typename ENTRY_T>
	class Scheduler
	{
		public:

			/**
			 * Base class for 'ENTRY_T' to support scheduling
			 */
			class Entry : public Double_list<ENTRY_T>::Entry
			{
				friend class Scheduler<ENTRY_T>;

				unsigned _time; /* time wich remains for current lap */

				/**
				 * Apply consumption of 'time'
				 */
				void _consume(unsigned const time)
				{ _time = _time > time ? _time - time : 0; }

				public:

					/**
					 * Constructor
					 */
					Entry() : _time(0) { }
			};

		protected:

			ENTRY_T * const _idle; /* Default entry, can't be removed */
			Double_list<ENTRY_T> _entries; /* List of entries beside '_idle' */
			unsigned const _lap_time; /* Time that an entry gets for one
			                           * scheduling lap to consume */

			ENTRY_T * _vm; /* Default entry, can't be removed */

		public:

			/**
			 * Constructor
			 */
			Scheduler(ENTRY_T * const idle, unsigned const lap_time)
			: _idle(idle), _lap_time(lap_time), _vm(0) { assert(_lap_time && _idle); }

			/**
			 * Returns the entry wich shall scheduled next
			 *
			 * \param t  At the call it contains the time, wich was consumed
			 *           by the last entry. At the return it is updated to
			 *           the next timeslice.
			 */
			ENTRY_T * next_entry(unsigned & t)
			{
				/* update current entry */
				ENTRY_T * e = _entries.head();
				if (!e) {
					t = _lap_time;
					return (_vm) ? _vm : _idle;
				}
				e->Entry::_consume(t);

				/* lookup entry with time > 0, refresh depleted timeslices */
				while (!e->Entry::_time) {
					e->Entry::_time = _lap_time;
					_entries.head_to_tail();
					e = _entries.head();
				}
#if 0
				ENTRY_T * ptr = _entries.head()->next();
				while(ptr) {
					if (ptr->priority() < e->priority()) {
						while (ptr != _entries.head())
							_entries.head_to_tail();
						t = 0;
						return next_entry(t);
					}
					ptr = ptr->next();
				}
#endif

				/* return next entry and appropriate portion of time */
				t = e->Entry::_time;
				return e;
			}

			/**
			 * Get the currently scheduled entry
			 */
			ENTRY_T * current_entry() const
			{
				if (!_entries.head())
					return _vm ? _vm : _idle;
				return _entries.head();
			}

			/**
			 * Ensure that 'e' does participate in scheduling afterwards
			 */
			void insert(ENTRY_T * const e)
			{
				if (e == _idle) return;
				e->Entry::_time = _lap_time;
				_entries.insert_tail(e);
			}

			/**
			 * Ensures that 'e' doesn't participate in scheduling afterwards
			 */
			void remove(ENTRY_T * const e) { _entries.remove(e); }

			/**
			 * Set remaining time of currently scheduled entry to 0
			 */
			void yield()
			{
				ENTRY_T * const e = _entries.head();
				if (e) e->_time = 0;
				return;
			}

			void vm(ENTRY_T * e) { _vm = e; }
	};

	class Schedule_context;
	typedef Scheduler<Schedule_context> Cpu_scheduler;

	/**
	 * Kernel object that can be scheduled for the CPU
	 */
	class Schedule_context : public Cpu_scheduler::Entry
	{
		private:

			unsigned char _prio;

		public:

			Schedule_context(unsigned char prio = 0) {}

			virtual void handle_exception() = 0;
			virtual void scheduled_next() = 0;
			virtual unsigned char priority() { return _prio; }
	};

	/**
	 * Sends requests to other IPC nodes, accumulates request announcments,
	 * provides serial access to them and replies to them if expected.
	 * Synchronizes communication.
	 *
	 * IPC node states:
	 *
	 *         +----------+                               +---------------+                             +---------------+
	 * --new-->| inactive |--send-request-await-reply---->| await reply   |               +--send-note--| prepare reply |
	 *         |          |<--receive-reply---------------|               |               |             |               |
	 *         |          |                               +---------------+               +------------>|               |
	 *         |          |<--request-is-a-note-------+---request-is-not-a-note------------------------>|               |
	 *         |          |<--------------------------(---not-await-request-----+                       |               |
	 *         |          |                           |   +---------------+     |                       |               |
	 *         |          |--await-request------------+-->| await request |<----+--send-reply-----------|               |
	 *         |          |--send-reply---------+-----+-->|               |--announce-request-+-------->|               |
	 *         |          |--send-note--+       |     |   +---------------+                   |         |               |
	 *         |          |             |       | request available                           |         |               |
	 *         |          |<------------+       |     |                                       |         |               |
	 *         |          |<--not-await-request-+     |                                       |         |               |
	 *         |          |<--request-is-a-note-------+---request-is-not-a-note---------------|-------->|               |
	 *         |          |<--request-is-a-note-----------------------------------------------+         |               |
	 *         +----------+                 +-------------------------+                                 |               |
	 *                                      | prepare and await reply |<--send-request-and-await-reply--|               |
	 *                                      |                         |--receive-reply----------------->|               |
	 *                                      +-------------------------+                                 +---------------+
	 *
	 * State model propagated to deriving classes:
	 *
	 *         +--------------+                                               +----------------+
	 * --new-->| has received |--send-request-await-reply-------------------->| awaits receipt |
	 *         |              |--await-request----------------------------+-->|                |
	 *         |              |                                           |   |                |
	 *         |              |<--request-available-----------------------+   |                |
	 *         |              |--send-reply-------------------------------+-->|                |
	 *         |              |--send-note--+                             |   |                |
	 *         |              |             |                             |   |                |
	 *         |              |<------------+                             |   |                |
	 *         |              |<--request-available-or-not-await-request--+   |                |
	 *         |              |<--announce-request----------------------------|                |
	 *         |              |<--receive-reply-------------------------------|                |
	 *         +--------------+                                               +----------------+
	 */
	class Ipc_node
	{
		/**
		 * IPC node states as depicted initially
		 */
		enum State
		{
			INACTIVE = 1,
			AWAIT_REPLY = 2,
			AWAIT_REQUEST = 3,
			PREPARE_REPLY = 4,
			PREPARE_AND_AWAIT_REPLY = 5,
		};

		/**
		 * Describes the buffer for incoming or outgoing messages
		 */
		struct Message_buf : public Fifo<Message_buf>::Element
		{
			void * base;
			size_t size;
			Ipc_node * origin;
		};

		Fifo<Message_buf> _request_queue; /* requests that waits to be
		                                   * received by us */
		Message_buf _inbuf; /* buffers message we have received lastly */
		Message_buf _outbuf; /* buffers the message we aim to send */
		State _state; /* current node state */

		/**
		 * Buffer next request from request queue in 'r' to handle it
		 */
		void _receive_request(Message_buf * const r);

		/**
		 * Receive a given reply if one is expected
		 *
		 * \param base  base of the reply payload
		 * \param size  size of the reply payload
		 */
		void _receive_reply(void * const base, size_t const size);

		/**
		 * Insert 'r' into request queue, buffer it if we were waiting for it
		 */
		void _announce_request(Message_buf * const r);

		/**
		 * Wether we expect to receive a reply message
		 */
		bool _awaits_reply();

		/**
		 * IPC node waits for a message to receive to its inbuffer
		 */
		virtual void _awaits_receipt() = 0;

		/**
		 * IPC node has received a message in its inbuffer
		 *
		 * \param s  size of the message
		 */
		virtual void _has_received(size_t const s) = 0;

		public:

			/**
			 * Construct an initially inactive IPC node
			 */
			Ipc_node();

			/**
			 * Destructor
			 */
			virtual ~Ipc_node() { }

			/**
			 * Send a request and wait for the according reply
			 *
			 * \param dest        targeted IPC node
			 * \param req_base    base of the request payload
			 * \param req_size    size of the request payload
			 * \param inbuf_base  base of the reply buffer
			 * \param inbuf_size  size of the reply buffer
			 */
			void send_request_await_reply(Ipc_node * const dest,
			                              void * const req_base,
			                              size_t const req_size,
			                              void * const inbuf_base,
			                              size_t const inbuf_size);

			/**
			 * Wait until a request has arrived and load it for handling
			 *
			 * \param inbuf_base  base of the request buffer
			 * \param inbuf_size  size of the request buffer
			 */
			void await_request(void * const inbuf_base,
			                   size_t const inbuf_size);

			/**
			 * Reply to last request if there's any
			 *
			 * \param reply_base  base of the reply payload
			 * \param reply_size  size of the reply payload
			 */
			void send_reply(void * const reply_base,
			                size_t const reply_size);

			/**
			 * Send a notification and stay inactive
			 *
			 * \param dest        targeted IPC node
			 * \param note_base   base of the note payload
			 * \param note_size   size of the note payload
			 *
			 * The caller must ensure that the note payload remains
			 * until it is buffered by the targeted node.
			 */
			void send_note(Ipc_node * const dest,
			               void * const note_base,
			               size_t const note_size);
	};

	/**
	 * Exclusive ownership and handling of one IRQ per instance at a max
	 */
	class Irq_owner : public Object_pool<Irq_owner>::Entry
	{
		/**
		 * To get any instance of this class by its ID
		 */
		typedef Object_pool<Irq_owner> Pool;
		static Pool * _pool() { static Pool _pool; return &_pool; }

		/**
		 * Is called when the IRQ we were waiting for has occured
		 */
		virtual void _received_irq() = 0;

		/**
		 * Is called when we start waiting for the occurence of an IRQ
		 */
		virtual void _awaits_irq() = 0;

		public:

			/**
			 * Translate 'Irq_owner_pool'-entry ID to IRQ ID
			 */
			static unsigned id_to_irq(unsigned id) { return id - 1; }

			/**
			 * Translate IRQ ID to 'Irq_owner_pool'-entry ID
			 */
			static unsigned irq_to_id(unsigned irq) { return irq + 1; }

			/**
			 * Constructor
			 */
			Irq_owner() : Pool::Entry(0) { }

			/**
			 * Destructor
			 */
			virtual ~Irq_owner() { }

			/**
			 * Ensure that our 'receive_irq' gets called on IRQ 'irq'
			 *
			 * \return  wether the IRQ is allocated to the caller or not
			 */
			bool allocate_irq(unsigned const irq);

			/**
			 * Release the ownership of the IRQ 'irq' if we own it
			 *
			 * \return  wether the IRQ is freed or not
			 */
			bool free_irq(unsigned const irq);

			/**
			 * If we own an IRQ, enable it and await 'receive_irq'
			 */
			void await_irq();

			/**
			 * Denote occurence of an IRQ if we own it and awaited it
			 */
			void receive_irq(unsigned const irq);

			/**
			 * Get owner of IRQ or 0 if the IRQ is not owned by anyone
			 */
			static Irq_owner * owner(unsigned irq);
	};

	/**
	 * Kernel representation of a user thread
	 */
	class Thread : public Cpu::User_context,
	               public Object<Thread, MAX_THREADS>,
	               public Schedule_context,
	               public Fifo<Thread>::Element,
	               public Ipc_node,
	               public Irq_owner
	{
		enum State { STOPPED, ACTIVE, AWAIT_IPC, AWAIT_RESUMPTION,
		             AWAIT_IRQ, AWAIT_SIGNAL };

		Platform_thread * const _platform_thread; /* userland object wich
		                                           * addresses this thread */
		State _state; /* thread state, description given at the beginning */
		Pagefault _pagefault; /* last pagefault triggered by this thread */
		Thread * _pager; /* gets informed if thread throws a pagefault */
		unsigned _pd_id; /* ID of the PD this thread runs on */
		Native_utcb * _phys_utcb; /* physical UTCB base */
		Native_utcb * _virt_utcb; /* virtual UTCB base */

		/**
		 * Resume execution
		 */
		void _activate();


		/**************
		 ** Ipc_node **
		 **************/

		void _has_received(size_t const s);

		void _awaits_receipt();


		/***************
		 ** Irq_owner **
		 ***************/

		void _received_irq() { _activate(); }

		void _awaits_irq();

		public:

			void * operator new (size_t, void * p) { return p; }

			/**
			 * Constructor
			 */
			Thread(Platform_thread * const platform_thread) :
				_platform_thread(platform_thread),
				_state(STOPPED), _pager(0), _pd_id(0),
				_phys_utcb(0), _virt_utcb(0)
			{ }

			/**
			 * Start this thread
			 *
			 * \param ip      instruction pointer to start at
			 * \param sp      stack pointer to use
			 * \param cpu_no  target cpu
			 *
			 * \retval  0  successful
			 * \retval -1  thread could not be started
			 */
			int start(void *ip, void *sp, unsigned cpu_no,
			          unsigned const pd_id, Native_utcb * const phys_utcb,
			          Native_utcb * const virt_utcb);

			/**
			 * Pause this thread
			 */
			void pause();

			/**
			 * Stop this thread
			 */
			void stop();

			/**
			 * Resume this thread
			 */
			int resume();

			/**
			 * Send a request and await the reply
			 */
			void request_and_wait(Thread * const dest, size_t const size);

			/**
			 * Wait for any request
			 */
			void wait_for_request();

			/**
			 * Reply to the last request
			 */
			void reply(size_t const size, bool const await_request);

			/**
			 * Initialize our execution context
			 *
			 * \param ip     instruction pointer
			 * \param sp     stack pointer
			 * \param pd_id  identifies protection domain we're assigned to
			 */
			void init_context(void * const ip, void * const sp,
			                  unsigned const pd_id);

			/**
			 * Handle a pagefault that originates from this thread
			 *
			 * \param va  virtual fault address
			 * \param w   if fault was caused by a write access
			 */
			void pagefault(addr_t const va, bool const w);

			/**
			 * Get unique thread ID, avoid method ambiguousness
			 */
			unsigned id() const { return Object::id(); }

			/**
			 * Gets called when we await a signal at a signal receiver
			 */
			void await_signal();

			/**
			 * Gets called when we have received a signal at a signal receiver
			 */
			void receive_signal(Signal const s);

			/**
			 * Handle the exception that currently blocks this thread
			 */
			void handle_exception();

			/**
			 * Continue executing this thread in userland
			 */
			void scheduled_next();


			/***************
			 ** Accessors **
			 ***************/

			Platform_thread * platform_thread() const {
				return _platform_thread; }

			void pager(Thread * const p) { _pager = p; }

			unsigned pd_id() const { return _pd_id; }

			Native_utcb * phys_utcb() const { return _phys_utcb; }
	};
}

#endif /* _CORE__KERNEL__THREAD_H_ */

