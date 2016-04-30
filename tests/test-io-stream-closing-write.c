#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "agent.h" 
#include "test-io-stream-common.h"

#include <stdlib.h>
#include <string.h>
#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#define NUM_MESSAGES 10

guint count = 0;
GMutex count_lock;
GCond count_cond;

static void
read_thread_cb (GInputStream *input_stream, TestIOStreamThreadData *data)
{
  gpointer tmp;
  guint stream_id;
  GError *error = NULL;
  gssize len;
  guint8 buf[MESSAGE_SIZE];

  /* Block on receiving some data. */
  len = g_input_stream_read (input_stream, buf, sizeof (buf), NULL, &error);
  g_assert_cmpint (len, ==, sizeof(buf));

  g_mutex_lock (&count_lock);
  count++;
  g_cond_broadcast (&count_cond);
  if (data->user_data) {
    g_mutex_unlock (&count_lock);
    return;
  }

  while (count != 4)
    g_cond_wait (&count_cond, &count_lock);
  g_mutex_unlock (&count_lock);

  /* Now we remove the stream, lets see how the writer handles that */

  tmp = g_object_get_data (G_OBJECT (data->other->agent), "stream-id");
  stream_id = GPOINTER_TO_UINT (tmp);

  nice_agent_remove_stream (data->other->agent, stream_id);
}

static void
write_thread_cb (GOutputStream *output_stream, TestIOStreamThreadData *data)
{
  gchar buf[MESSAGE_SIZE] = {0};
  gssize ret;
  GError *error = NULL;

  g_mutex_lock (&count_lock);
  count++;
  g_cond_broadcast (&count_cond);
  g_mutex_unlock (&count_lock);

  do {
    g_assert_no_error (error);
    ret = g_output_stream_write (output_stream, buf, sizeof (buf), NULL,
        &error);

    if (!data->user_data) {
      g_assert_cmpint (ret, ==, sizeof (buf));
      return;
    }
  } while (ret > 0);
  g_assert_cmpint (ret, ==, -1);

  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED);
  g_clear_error (&error);

  stop_main_loop (data->error_loop);
}

int main (void)
{
  const TestIOStreamCallbacks callbacks = {
    read_thread_cb,
    write_thread_cb,
    NULL,
    NULL,
  };

#ifdef G_OS_WIN32
  WSADATA w;
  WSAStartup (0x0202, &w);
#endif
  g_type_init ();
  g_thread_init (NULL);

  run_io_stream_test (30, TRUE, &callbacks, (gpointer) TRUE, NULL, NULL, NULL);

#ifdef G_OS_WIN32
  WSACleanup ();
#endif

  return 0;
}
