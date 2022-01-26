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
const char *const CLIENT_BUS_NAME = "alex.client";
const char *const SERVER_OBJECT_PATH_NAME = "/in/alex/fc";
const char *const CLIENT_OBJECT_PATH_NAME = "/in/alex/client";
const char *const METHOD_NAME = "first_char";

DBusError dbus_error;
void print_dbus_error(char *str) {
  fprintf(stderr, "%s: %s\n", str, dbus_error.message);
  dbus_error_free(&dbus_error);
}

int main(int argc, char **argv) {
  DBusConnection *conn;
  int ret;
  char input[80];

  dbus_error_init(&dbus_error);

  // Get a connection to the bus
  conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

  if (dbus_error_is_set(&dbus_error))
    print_dbus_error("dbus_bus_get");

  if (!conn)
    exit(1);

  // Handle client input
  printf("Please enter a message: ");
  while (fgets(input, 78, stdin) != NULL) {
    // Get a well known name
    while (1) {
      // Ask the bus to register a name
      ret = dbus_bus_request_name(conn, CLIENT_BUS_NAME, 0, &dbus_error);

      if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
        break;

      if (ret == DBUS_REQUEST_NAME_REPLY_IN_QUEUE) {
        fprintf(stderr, "Waiting for the bus ... \n");
        sleep(1);
        continue;
      }
      if (dbus_error_is_set(&dbus_error))
        print_dbus_error("dbus_bus_get");
    }

    DBusMessage *request;

    // Construct a dbus message
    if ((request = dbus_message_new_method_call(SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME,
                                                INTERFACE_NAME, METHOD_NAME)) == NULL) {
      fprintf(stderr, "Error in dbus_message_new_method_call\n");
      exit(1);
    }

    // Reading the arguments of the method call
    DBusMessageIter iter;
    dbus_message_iter_init_append(request, &iter);
    char *ptr = input;
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &ptr)) {
      fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
      exit(1);
    }

    // Send the message and get a handle for a reply
    DBusPendingCall *pending_return;
    if (!dbus_connection_send_with_reply(conn, request, &pending_return, -1)) {
      fprintf(stderr, "Error in dbus_connection_send_with_reply\n");
      exit(1);
    }

    if (pending_return == NULL) {
      fprintf(stderr, "pending return is NULL");
      exit(1);
    }

    // Blocks until message queue is empty
    dbus_connection_flush(conn);

    dbus_message_unref(request);

    // Block until we receive a reply
    dbus_pending_call_block(pending_return);

    // Get the reply message
    DBusMessage *reply;
    if ((reply = dbus_pending_call_steal_reply(pending_return)) == NULL) {
      fprintf(stderr, "Error in dbus_pending_call_steal_reply");
      exit(1);
    }

    dbus_pending_call_unref(pending_return);

    // Read the arguments
    char *s;
    if (dbus_message_get_args(reply, &dbus_error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
      printf("%s\n", s);
    } else {
      fprintf(stderr, "Did not get arguments in reply\n");
      exit(1);
    }
    dbus_message_unref(reply);

    if (dbus_bus_release_name(conn, CLIENT_BUS_NAME, &dbus_error) == -1) {
      fprintf(stderr, "Error in dbus_bus_release_name\n");
      exit(1);
    }

    printf("Please enter a message: ");
  }

  return 0;
}
