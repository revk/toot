// Tooting app

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <err.h>
#include <ajlcurl.h>

int debug = 0;

int
main (int argc, const char *argv[])
{
   const char *auth_file = "/etc/tootauth.json";
   const char *server = NULL;
   const char *create_app = NULL;
   const char *code = NULL;
   const char *website = NULL;
   const char *language = "en";
   const char *visibility = "public";
   const char *spoiler = NULL;
   const char *reply = NULL;
   const char *scheduled = NULL;
   const char *redirect = NULL;
   const char *scope = "read write follow push";
   const char *edit = NULL;
   const char *delete = NULL;
   const char *boost = NULL;
   const char *unboost = NULL;
   const char *like = NULL;
   const char *unlike = NULL;
   const char *bookmark = NULL;
   const char *unbookmark = NULL;
   const char *attach = NULL;
   const char *focus = "0,0";
   const char *poll = NULL;
   int polltime = 24 * 60 * 60;
   const char *media = NULL;
   char *status = NULL;
   int login = 0;
   {                            // POPT
      poptContext optCon;       // context for parsing command-line options
      const struct poptOption optionsTable[] = {
         {"status", 0, POPT_ARG_STRING, &status, 0, "Status",
          "Text of status, or - for stdin, assumes a post if no --id, else an edit"},
         {"attach", 0, POPT_ARG_STRING, &attach, 0, "Attach", "Comma separated media IDs"},
         {"focus", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &focus, 0, "Focus", "x,y"},
         {"poll", 0, POPT_ARG_STRING, &poll, 0, "Poll", "Comma separated poll strings"},
         {"poll-time", 0, POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &polltime, 0, "N", "Poll time"},
         {"boost", 0, POPT_ARG_STRING, &boost, 0, "Boost", "ID of post for boost"},
         {"unboost", 0, POPT_ARG_STRING, &unboost, 0, "UnBoost", "ID of post for unboost"},
         {"like", 0, POPT_ARG_STRING, &like, 0, "Like", "ID of post for like"},
         {"unlike", 0, POPT_ARG_STRING, &unlike, 0, "UnLike", "ID of post for unlike"},
         {"bookmark", 0, POPT_ARG_STRING, &bookmark, 0, "Bookmark", "ID of post for bookmark"},
         {"unbookmark", 0, POPT_ARG_STRING, &unbookmark, 0, "UnBookmark", "ID of post for unlike"},
         {"scheduled", 0, POPT_ARG_STRING, &scheduled, 0, "Scheduled", "Scheduled post"},
         {"edit", 0, POPT_ARG_STRING, &edit, 0, "Edit", "ID of post for edit"},
         {"delete", 0, POPT_ARG_STRING, &delete, 0, "Delete", "ID of post for delete"},
         {"reply", 0, POPT_ARG_STRING, &reply, 0, "Reply", "ID of post we are replying to"},
         {"spolier", 0, POPT_ARG_STRING, &spoiler, 0, "Spoiler", "Spolier for sensitive post"},
         {"visibility", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &visibility, 0, "Visibility",
          "public/unlisted/private/direct"},
         {"language", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &language, 0, "Language", "ISO639"},
         {"media", 0, POPT_ARG_STRING, &media, 0, "media", "filename"},
         {"auth-file", 'A', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &auth_file, 0, "Auth file", "filename"},
         {"login", 0, POPT_ARG_NONE, &login, 0, "Authorise a user (returns URL)"},
         {"code", 0, POPT_ARG_STRING, &code, 0, "Create auth for use from code", "code"},
         {"server", 0, POPT_ARG_STRING, &server, 0, "Server", "hostname"},
         {"create-app", 0, POPT_ARG_STRING, &create_app, 0, "Create app", "App name"},
         {"redirect", 0, POPT_ARG_STRING, &redirect, 0, "Redirect URL for login", "URL"},
         {"scope", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &scope, 0, "Scope", "scopes"},
         {"website", 0, POPT_ARG_STRING, &website, 0, "Website (for created app)", "URL"},
         {"debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug"},
         POPT_AUTOHELP {}
      };

      optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp (optCon, "text");

      int c;
      if ((c = poptGetNextOpt (optCon)) < -1)
         errx (1, "%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

      if (poptPeekArg (optCon))
      {
         if (status)
         {
            poptPrintUsage (optCon, stderr, 0);
            return -1;
         }
         // Status from args
         size_t len;
         FILE *f = open_memstream (&status, &len);
         const char *v;
         int n = 0;
         while ((v = poptGetArg (optCon)))
            fprintf (f, "%s%s", n++ ? " " : "", v);
         fclose (f);
      }
      poptFreeContext (optCon);
   }
   CURL *curl = curl_easy_init ();
   if (debug)
      curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L);

   if (access (auth_file, F_OK) && !create_app)
      errx (1, "No auth file (%s), maybe you need to --create-app", auth_file);
   if (create_app)
   {                            // Create an app
      if (!server)
         errx (1, "Specify --server");
      if (!redirect)
         redirect = "urn:ietf:wg:oauth:2.0:oob";
      j_t t = j_create (),
         r = j_create ();
      j_store_string (t, "client_name", create_app);
      j_store_string (t, "redirect_uris", redirect);
      j_store_string (t, "scopes", scope);
      if (website)
         j_store_string (t, "website", website);
      if (debug)
         j_err (j_write_pretty (t, stderr));
      char *e = j_curl_send (curl, t, r, NULL, "https://%s/api/v1/apps", server);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_store_string (r, "server", server);
      j_store_string (r, "scope", scope);
      e = j_write_file (r, auth_file);
      if (e)
         errx (1, "Failed to create %s: %s", auth_file, e);
      j_delete (&t);
      j_delete (&r);
      login = 1;
   }
   j_t auth = j_create ();
   j_err (j_read_file (auth, auth_file));
   server = j_get (auth, "server");
   if (!server)
      errx (1, "Need to --create-app");
   if (login)
   {                            // Auth a user
      size_t l;
      char *url = NULL;
      FILE *m = open_memstream (&url, &l);
      fprintf (m, "https://%s/oauth/authorize?", server);
      void add (const char *tag, const char *val)
      {
         fprintf (m, "%s=", tag);
         while (*val)
            if (isalnum (*val))
               fputc (*val++, m);
            else
               fprintf (m, "%%%02X", *val++);
         fprintf (m, "&");
      }
      add ("response_type", "code");
      add ("client_id", j_get (auth, "client_id"));
      add ("redirect_uri", j_get (auth, "redirect_uri"));
      add ("scope", j_get (auth, "scope"));
      add ("force_login", "true");
      add ("lang", language);
      fclose (m);
      url[--l] = 0;             // Trailing &
      printf ("%s", url);
      return 0;
   }
   if (code)
   {                            // Get a token
      j_t t = j_create (),
         r = j_create ();
      if (code)
         j_store_string (t, "code", code);
      j_store_string (t, "client_id", j_get (auth, "client_id"));
      j_store_string (t, "client_secret", j_get (auth, "client_secret"));
      j_store_string (t, "redirect_uri", j_get (auth, "redirect_uri"));
      j_store_string (t, "grant_type", "authorization_code");
      j_store_string (t, "scope", j_get (auth, "scope"));
      if (debug)
         j_err (j_write_pretty (t, stderr));
      char *e = j_curl_send (curl, t, r, NULL, "https://%s/oauth/token", server);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      const char *bearer = j_get (r, "access_token");
      if (!bearer || !*bearer || strcmp (j_get (r, "token_type") ? : "", "Bearer"))
         errx (1, "Expecting bearer auth");
      j_store_string (auth, "access_token", bearer);
      j_store_string (auth, "created_at", j_get (r, "created_at"));
      j_store_string (auth, "scope", j_get (r, "scope"));
      j_store_string (auth, "username", j_get (r, "account.username"));
      j_store_string (auth, "display_name", j_get (r, "account.display_name"));
      j_delete (&t);
      j_delete (&r);
      e = j_write_file (auth, auth_file);
      if (e)
         errx (1, "Failed to create %s: %s", auth_file, e);
      return 0;
   }
   const char *bearer = j_get (auth, "access_token");
   if (!bearer)
      errx (1, "Need --login or --code");
   {                            // Verify
      j_t r = j_create ();
      char *e = j_curl_get (curl, NULL, r, bearer, "https://%s/api/v1/apps/verify_credentials", server);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         bearer = NULL;         // Get new creds
      if (debug)
         j_delete (&r);
   }
   if (media)
   {                            // Media upload
      if (!status || !*status)
         errx (1, "Include status to describe the media");
      j_t t = j_create (),
         r = j_create ();
      j_t f = j_store_object (t, "file");
      j_store_string (f, "file", media);
      char *d = strrchr (media, '.');
      if (d)
      {
         if (!strcasecmp (d, ".png"))
            j_store_string (f, "type", "image/png");
         else if (!strcasecmp (d, ".jpg") || !strcasecmp (d, ".jpeg"))
            j_store_string (f, "type", "image/jpeg");
      }
      d = strchr (media, '/');
      j_store_string (f, "filename", d ? d + 1 : media);
      j_store_string (t, "description", status);
      j_store_string (t, "focus", focus);
      if (debug)
         j_err (j_write_pretty (t, stderr));
      char *e = j_curl_form (curl, t, r, bearer, "https://%s/api/v2/media", server);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      printf ("%s", j_get (r, "id"));
      j_delete (&t);
      j_delete (&r);
   } else if (status)
   {
      if (!*status)
         errx (1, "Empty status");
      if (!strcmp (status, "-"))
      {                         // Read status from stdin
         size_t l,
           s;
         FILE *m = open_memstream (&status, &l);
         char buf[10240];
         while ((s = read (fileno (stdin), buf, sizeof (buf))) >= 0)
            fwrite (buf, s, 1, m);
         fclose (m);
      }
      j_t t = j_create (),
         r = j_create ();
      // TODO Idempotency-Key
      j_store_string (t, "status", status);
      if (reply)
         j_store_string (t, "in_reply_to_id", reply);
      if (spoiler)
      {
         j_store_boolean (t, "sensitive", 1);
         if (*spoiler)
            j_store_string (t, "spoiler_text", spoiler);
      }
      j_store_string (t, "visibility", visibility);
      j_store_string (t, "language", language);
      if (scheduled)
         j_store_string (t, "scheduled_at", scheduled);
      if (attach)
      {
         j_t m = j_store_array (t, "media_ids");
         char *a = strdupa (attach);
         while (a && *a)
         {
            char *b = strchr (a, ',');
            if (b)
               *b++ = 0;
            j_append_string (m, a);
            a = b;
         }
      }
      if (poll)
      {
         j_t p = j_store_object (t, "poll");
         j_store_int (p, "expires_in", polltime);
         j_t o = j_store_array (p, "options");
         char *a = strdupa (poll);
         while (a && *a)
         {
            char *b = strchr (a, ',');
            if (b)
               *b++ = 0;
            j_append_string (o, a);
            a = b;
         }
      }
      if (debug)
         j_err (j_write_pretty (t, stderr));
      char *e;
      if (edit)
         e = j_curl_put (curl, t, r, bearer, "https://%s/api/v1/statuses/%s", server, edit);
      else
         e = j_curl_send (curl, t, r, bearer, "https://%s/api/v1/statuses", server);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      printf ("%s", j_get (r, "id"));
      j_delete (&t);
      j_delete (&r);
   }
   if (delete)
   {
      j_t r = j_create ();
      char *e = j_curl_delete (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s", server, delete);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   if (boost)
   {
      j_t r = j_create ();
      char *e = j_curl_post (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s/reblog", server, boost);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   if (unboost)
   {
      j_t r = j_create ();
      char *e = j_curl_post (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s/unreblog", server, unboost);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   if (like)
   {
      j_t r = j_create ();
      char *e = j_curl_post (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s/favourite", server, like);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   if (unlike)
   {
      j_t r = j_create ();
      char *e = j_curl_post (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s/unfavourite", server, unlike);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   if (bookmark)
   {
      j_t r = j_create ();
      char *e = j_curl_post (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s/bookmark", server, bookmark);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   if (unbookmark)
   {
      j_t r = j_create ();
      char *e = j_curl_post (curl, NULL, r, bearer, "https://%s/api/v1/statuses/%s/unbookmark", server, unbookmark);
      if (debug)
         j_err (j_write_pretty (r, stderr));
      if (e)
         errx (1, "Failed %s", e);
      j_delete (&r);
   }
   j_delete (&auth);
   curl_easy_cleanup (curl);
   return 0;
}