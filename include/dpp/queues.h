#pragma once
#include <unordered_map>
#include <string>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>

namespace dpp {

/** Encodes a url parameter similar to php urlencode() */
std::string url_encode(const std::string &value);

/** Error values. Don't change the order or add extra values here,
 * as they map onto the error values of cpp-httplib
 */
enum http_error {
	h_success = 0,
	h_unknown,
	h_connection,
	h_bind_ip_address,
	h_read,
	h_write,
	h_exceed_redirect_count,
	h_canceled,
	h_ssl_connection,
	h_ssl_loading_certs,
	h_ssl_server_verification,
	h_unsupported_multipart_boundary_chars,
	h_compression,
};

/** The result of any HTTP request. Contains the headers, vital
 * rate limit figures, and returned request body.
 */
struct http_request_completion_t {
	/** HTTP headers of response */
	std::map<std::string, std::string> headers;
	/** HTTP status, e.g. 200 = OK, 404 = Not found, 429 = Rate limited */
	uint16_t status = 0;
	/** Error status (e.g. if the request could not connect at all) */
	http_error error = h_success;
	/** Ratelimit bucket */
	std::string ratelimit_bucket;
	/** Ratelimit limit of requests */
	uint64_t ratelimit_limit = 0;
	/** Ratelimit remaining requests */
	uint64_t ratelimit_remaining = 0;
	/** Ratelimit reset after (seconds) */
	uint64_t ratelimit_reset_after = 0;
	/** Ratelimit retry after (seconds) */
	uint64_t ratelimit_retry_after = 0;
	/** True if this request has caused us to be globally rate limited */
	bool ratelimit_global = false;
	/** Reply body */
	std::string body;
};

/** Results of HTTP requests are called back to these std::function types.
 * NOTE: Returned http_completion_events are called ASYNCRONOUSLY in your
 * code which means they execute in a separate thread. The completion events
 * arrive in order.
 */
typedef std::function<void(const http_request_completion_t&)> http_completion_event;

/** Various types of http method supported by the Discord API
 */
enum http_method {
	m_get, m_post, m_put, m_patch, m_delete
};

/** A HTTP request. You should instantiate one of these objects via its constructor,
 * and pass a pointer to it into an instance of request_queue. Although you can
 * directly call hthe Run() method of the object and it will make a HTTP call, be
 * aware that if you do this, it will be a BLOCKING call (not asynchronous) and
 * will not respect rate limits, as both of these functions are managed by the
 * request_queue class.
 */
class http_request {
	/** Completion callback */
	http_completion_event complete_handler;
	/** True if request has been made */
	bool completed;
public:
	/** Endpoint name e.g. /api/users */
	std::string endpoint;
	/** Major and minor parameters */
	std::string parameters;
	/** Postdata for POST and PUT */
	std::string postdata;
	/** HTTP method for request */
	http_method method;

	/** Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _endpoint The API endpoint, e.g. /api/guilds
	 * @param _parameters Major and minor parameters for the endpoint e.g. a user id or guild id
	 * @param completion completion event to call when done
	 * @param _postdata Data to send in POST and PUT requests
	 * @param method The HTTP method to use from dpp::http_method
	 */
	http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get);

	/** Destructor */
	~http_request();

	/** Call the completion callback, if the request is complete.
	 * @param c callback to call
	 */
	void complete(const http_request_completion_t &c);

	/** Execute the HTTP request and mark the request complete.
	 * @param owner creating cluster
	 */
	http_request_completion_t Run(const class cluster* owner);

	/** Returns true if the request is complete */
	bool is_completed();
};

/** A rate limit bucket. The library builds one of these for
 * each endpoint.
 */
struct bucket_t {
	/** Request limit */
	uint64_t limit;
	/** Requests remaining */
	uint64_t remaining;
	/** Ratelimit of this bucket resets after this many seconds */
	uint64_t reset_after;
	/** Ratelimit of this bucket can be retried after this many seconds */
	uint64_t retry_after;
	/** Timestamp this buckets counters were updated */
	time_t timestamp;
};

/** The request_queue class manages rate limits and marshalls HTTP requests that have
 * been built as http_request objects. It ensures asynchronous delivery of events and
 * queueing of requests.
 *
 * It will spawn two threads, one to make outbound HTTP requests and push the returned
 * results into a queue, and the second to call the callback methods with these results.
 * They are separated so that if the user decides to take a long time processing a reply
 * in their callback it won't affect when other requests are sent, and if a HTTP request
 * takes a long time due to latency, it won't hold up user processing.
 *
 * There is usually only one request_queue object in each dpp::cluster, which is used
 * internally for the various REST methods such as sending messages.
 */
class request_queue {
private:
	/** The cluster that owns this request_queue */
	const class cluster* creator;
	/** Mutexes for thread safety */
	std::mutex in_mutex;
	std::mutex out_mutex;
	/** In and out threads */
	std::thread* in_thread;
	std::thread* out_thread;
	/** Ratelimit bucket counters */
	std::map<std::string, bucket_t> buckets;
	/** Queue of requests to be made */
	std::map<std::string, std::vector<http_request*>> requests_in;
	/** Completed requests queue */
	std::queue<std::pair<http_request_completion_t*, http_request*>> responses_out;
	/** Set to true if the threads should terminate */
	bool terminating;
	/** True if globally rate limited - makes the entire request thread wait */
	bool globally_ratelimited;
	/** How many seconds we are globally rate limited for, if globally_ratelimited is true */
	uint64_t globally_limited_for;

	/** Ports for notifications of request completion.
	 * Why are we using sockets here instead of std::condition_variable? Because
	 * in the future we will want to notify across clusters of completion and state,
	 * and we can't do this across processes with condition variables.
	 */
	int in_queue_port;
	int out_queue_port;
	int in_queue_listen_sock;
	int in_queue_connect_sock;
	int out_queue_listen_sock;
	int out_queue_connect_sock;

	/** Thread loop functions */
	void in_loop();
	void out_loop();

	/** Notify request thread of a new request */
	void emit_in_queue_signal();

	/** Notify completion thread of new completed request */
	void emit_out_queue_signal();
public:
	/** Constructor
	 * @param owner The creating cluster
	 */
	request_queue(const class cluster* owner);

	/** Destructor */
	~request_queue();

	/** Put a http_request into the request queue. You should ALWAYS "new" an object
	 * to pass to here -- don't submit an object that's on the stack!
	 * @param req request to add
	 */
	void post_request(http_request *req);
};

};
