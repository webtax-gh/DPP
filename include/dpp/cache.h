#pragma once

#include <dpp/discord.h>
#include <map>
#include <mutex>

namespace dpp {

	/** A cache object maintains a cache of dpp::managed objects.
	 * This is for example users, channels or guilds.
	 */
	class cache {
	private:

		/** Mutex to protect the cache */
		std::mutex cache_mutex;

		/** Cached items */
		std::unordered_map<uint64_t, managed*> cache_map;

	public:

		/** Store an object in the cache.
		 * @param object object to store
		 */
		void store(managed* object);

		/** Remove an object from the cache.
		 * @param object object to remove
		 */
		void remove(managed* object);

		/** Find an object in the cache by id.
		 * @param id Object id to find
		 */
		managed* find(snowflake id);

		/** Return a count of the number of items in the cache.
		 */
		uint64_t count();
	};

	/** Run garbage collection across all caches removing deleted items
	 * that have been deleted over 60 seconds ago.
	 */
	void garbage_collection();

	#define cache_decl(type, setter, getter, counter) type * setter (snowflake id); cache * getter (); uint64_t counter ();

	/* Declare major caches */
	cache_decl(user, find_user, get_user_cache, get_user_count);
	cache_decl(guild, find_guild, get_guild_cache, get_guild_count);
	cache_decl(role, find_role, get_role_cache, get_role_count);
	cache_decl(channel, find_channel, get_channel_cache, get_channel_count);
	cache_decl(emoji, find_emoji, get_emoji_cache, get_emoji_count);
};

