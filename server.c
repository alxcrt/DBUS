#include <ctype.h>
#include <dbus/dbus.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// DBUS constants
const char *const INTERFACE_NAME = "alex.dbus";
const char *const SERVER_BUS_NAME = "alex.server";
const char *const OBJECT_PATH_NAME = "/in/alex/fc";
const char *const METHOD_NAME = "first_char";

DBusError dbus_error;

void print_dbus_error(char *str) {
  fprintf(stderr, "%s: %s\n", str, dbus_error.message);
  dbus_error_free(&dbus_error);
}

int main(int argc, char **argv) {
  // Get a connection to the bus
  DBusConnection *conn;
  int ret;

  // Initialize the error
  dbus_error_init(&dbus_error);

  // Get a connection to the bus
  conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

  // Check for errors
  if (dbus_error_is_set(&dbus_error))
    print_dbus_error("dbus_bus_get");

  // Check if we have a connection
  if (!conn)
    exit(1);

  // Get a well known name
  ret = dbus_bus_request_name(conn, SERVER_BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);

  // Check for errors
  if (dbus_error_is_set(&dbus_error))
    print_dbus_error("dbus_bus_get");

  // Check if we are connected to the bus
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    fprintf(stderr, "Dbus: not primary owner, ret = %d\n", ret);
    exit(1);
  }

  // Handle request from clients
  while (1) {
    // Get a message from client
    if (!dbus_connection_read_write_dispatch(conn, -1)) {
      fprintf(stderr, "Not connected now.\n");
      exit(1);
    }

    DBusMessage *message;

    // Get the message from the client
    if ((message = dbus_connection_pop_message(conn)) == NULL) {
      fprintf(stderr, "Did not get message\n");
      continue;
    }

    // Check if the message is a method call
    if (dbus_message_is_method_call(message, INTERFACE_NAME, METHOD_NAME)) {
      char *s;
      bool error = false;

      // Get the parameters
      if (dbus_message_get_args(message, &dbus_error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
        printf("%s", s);

        if (!error) {
          // send reply
          DBusMessage *reply;
          char answer[50];

          s[strlen(s) - 1] = '\0';

          sprintf(answer, "Message from server: First char of %s is %c", s, s[0]);
          if ((reply = dbus_message_new_method_return(message)) == NULL) {
            fprintf(stderr, "Error in dbus_message_new_method_return\n");
            exit(1);
          }

          DBusMessageIter iter;
          dbus_message_iter_init_append(reply, &iter);
          char *ptr = answer;
          if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &ptr)) {
            fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
            exit(1);
          }

          if (!dbus_connection_send(conn, reply, NULL)) {
            fprintf(stderr, "Error in dbus_connection_send\n");
            exit(1);
          }

          dbus_connection_flush(conn);

          dbus_message_unref(reply);
        } else  // There was an error
        {
          DBusMessage *dbus_error_msg;
          char error_msg[] = "Error in input";
          if ((dbus_error_msg = dbus_message_new_error(message, DBUS_ERROR_FAILED, error_msg)) == NULL) {
            fprintf(stderr, "Error in dbus_message_new_error\n");
            exit(1);
          }

          if (!dbus_connection_send(conn, dbus_error_msg, NULL)) {
            fprintf(stderr, "Error in dbus_connection_send\n");
            exit(1);
          }

          dbus_connection_flush(conn);

          dbus_message_unref(dbus_error_msg);
        }
      } else {
        print_dbus_error("Error getting message");
      }
    }
  }

  return 0;
}
