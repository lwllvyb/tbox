/*!The Treasure Box Library
 * 
 * TBox is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 * 
 * TBox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with TBox; 
 * If not, see <a href="http://www.gnu.org/licenses/"> http://www.gnu.org/licenses/</a>
 * 
 * Copyright (C) 2009 - 2017, ruki All rights reserved.
 *
 * @author      ruki
 * @file        thread.c
 * @ingroup     platform
 */

/* //////////////////////////////////////////////////////////////////////////////////////
 * includes
 */
#include "thread.h"
#include "atomic.h"
#include "time.h"
#include "thread_local.h"
#include "impl/thread_local.h"

/* //////////////////////////////////////////////////////////////////////////////////////
 * types
 */

// the return value type of the thread function
#ifdef TB_CONFIG_OS_WINDOWS
typedef tb_uint32_t     tb_thread_retval_t;
#else
typedef tb_pointer_t    tb_thread_retval_t;
#endif

/* //////////////////////////////////////////////////////////////////////////////////////
 * declaration
 */

/* //////////////////////////////////////////////////////////////////////////////////////
 * private implementation
 */
static tb_bool_t tb_thread_local_free(tb_iterator_ref_t iterator, tb_pointer_t item, tb_cpointer_t priv)
{
    // the local
    tb_thread_local_ref_t local = (tb_thread_local_ref_t)item;
    if (local) 
    { 
        // free the thread local data
        if (local->free) 
        {
            // free it
            local->free(tb_thread_local_get(local));
        }
    }

    // ok
    return tb_true;
}
static tb_thread_retval_t tb_thread_func(tb_pointer_t priv)
{
    // done
    tb_thread_retval_t  retval = (tb_thread_retval_t)0;
    tb_value_ref_t      args = (tb_value_ref_t)priv;
    do
    {
        // check
        tb_assert_and_check_break(args);

        // get the thread function
        tb_thread_func_t func = (tb_thread_func_t)args[0].ptr;
        tb_assert_and_check_break(func);

        // call the thread function
        retval = (tb_thread_retval_t)(tb_size_t)func(args[1].ptr);

        // free all thread local data on the current thread
        tb_thread_local_walk(tb_thread_local_free, tb_null);

    } while (0);

    // exit arguments
    if (args) tb_free(args);
    args = tb_null;

    // return the return value
    return retval;
}

/* //////////////////////////////////////////////////////////////////////////////////////
 * implementation
 */
#ifdef TB_CONFIG_OS_WINDOWS
#   include "windows/thread.c"
#elif defined(TB_CONFIG_POSIX_HAVE_PTHREAD_CREATE)
#   include "posix/thread.c"
#else
tb_thread_ref_t tb_thread_init(tb_char_t const* name, tb_thread_func_t func, tb_cpointer_t priv, tb_size_t stack)
{
    tb_trace_noimpl();
    return tb_null;
}
tb_void_t tb_thread_exit(tb_thread_ref_t thread)
{
    tb_trace_noimpl();
}
tb_long_t tb_thread_wait(tb_thread_ref_t thread, tb_long_t timeout, tb_int_t* retval)
{
    tb_trace_noimpl();
    return -1;
}
tb_void_t tb_thread_return(tb_int_t value)
{
    tb_trace_noimpl();
}
tb_bool_t tb_thread_suspend(tb_thread_ref_t thread)
{
    tb_trace_noimpl();
    return tb_false;
}
tb_bool_t tb_thread_resume(tb_thread_ref_t thread)
{
    tb_trace_noimpl();
    return tb_false;
}
tb_size_t tb_thread_self()
{
    tb_trace_noimpl();
    return 0;
}
#endif
tb_bool_t tb_thread_once(tb_atomic_t* lock, tb_bool_t (*func)(tb_cpointer_t), tb_cpointer_t priv)
{
    // check
    tb_check_return_val(lock && func, tb_false);

    /* called?
     *
     * 0: have been not called
     * 1: be calling
     * 2: have been called and ok
     * -2: have been called and failed
     */
    tb_atomic_t called = tb_atomic_fetch_and_pset(lock, 0, 1);

    // called?
    if (called && called != 1) return called == 2;
    // have been not called? call it
    else if (!called)
    {
        // call the once function
        tb_bool_t ok = func(priv);

        // call ok
        tb_atomic_set(lock, ok? 2 : -1);

        // ok?
        return ok;
    }
    // calling? wait it
    else
    {
        // try getting it
        tb_size_t tryn = 50;
        while ((1 == tb_atomic_get(lock)) && tryn--)
        {
            // wait some time
            tb_msleep(100);
        }
    }

    // ok? 1: timeout, -2: failed, 2: ok
    return tb_atomic_get(lock) == 2;
}
