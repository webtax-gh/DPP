#include <dpp/discord.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::map<std::string, dpp::guild_flags> featuremap = {
	{"INVITE_SPLASH", dpp::g_invite_splash },
	{"VIP_REGIONS", dpp::g_vip_regions },
	{"VANITY_URL", dpp::g_vanity_url },
	{"VERIFIED", dpp::g_verified },
	{"PARTNERED", dpp::g_partnered },
	{"COMMUNITY", dpp::g_community },
	{"COMMERCE", dpp::g_commerce },
	{"NEWS", dpp::g_news },
	{"DISCOVERABLE", dpp::g_discoverable },
	{"FEATUREABLE", dpp::g_featureable },
	{"ANIMATED_ICON", dpp::g_animated_icon },
	{"BANNER", dpp::g_banner },
	{"WELCOME_SCREEN_ENABLED", dpp::g_welcome_screen_enabled },
	{"MEMBER_VERIFICATION_GATE_ENABLED", dpp::g_member_verification_gate },
	{"PREVIEW_ENABLED", dpp::g_preview_enabled }
};

std::map<std::string, dpp::region> regionmap = {
	{ "brazil", dpp::r_brazil },
	{ "central-europe", dpp::r_central_europe },
	{ "hong-kong", dpp::r_hong_kong },
	{ "india", dpp::r_india },
	{ "japan",  dpp::r_japan },
	{ "russia", dpp::r_russia },
	{ "singapore", dpp::r_singapore },
	{ "south-africa", dpp::r_south_africa },
	{ "sydney", dpp::r_sydney },
	{ "us-central", dpp::r_us_central },
	{ "us-east", dpp::r_us_east },
	{ "us-south", dpp::r_us_south },
	{ "us-west", dpp::r_us_west },
	{ "western-europe", dpp::r_western_europe }
};


namespace dpp {

guild::guild() :
	managed(),
	flags(0),
	owner_id(0),
	voice_region(r_us_central),
	afk_channel_id(0),
	afk_timeout(0),
	widget_channel_id(0),
	verification_level(0),
	default_message_notifications(0),
	explicit_content_filter(0),
	mfa_level(0),
	application_id(0),
	system_channel_id(0),
	rules_channel_id(0),
	member_count(0),
	premium_tier(0),
	premium_subscription_count(0),
	public_updates_channel_id(0),
	max_video_channel_users(0)

{
}

guild::~guild()
{
}

guild_member::guild_member() :
	joined_at(0),
	premium_since(0),
	flags(0)

{
}

guild_member::~guild_member()
{
}

guild_member& guild_member::fill_from_json(nlohmann::json* j, const guild* g, const user* u) {
	this->guild_id = g->id;
	this->user_id = u->id;
	this->nickname = StringNotNull(j, "nickname");
	this->joined_at = TimestampNotNull(j, "joined_at");
	this->premium_since = TimestampNotNull(j, "premium_since");
	for (auto & role : (*j)["roles"]) {
		this->roles.push_back(from_string<uint64_t>(role.get<std::string>(), std::dec));
	}
	this->flags |= BoolNotNull(j, "deaf") ? dpp::gm_deaf : 0;
	this->flags |= BoolNotNull(j, "mute") ? dpp::gm_mute : 0;
	this->flags |= BoolNotNull(j, "pending") ? dpp::gm_pending : 0;
	return *this;
}

bool guild::is_large() const {
	return this->flags & g_large;
}

bool guild::is_unavailable() const {
	return this->flags & g_unavailable;
}

bool guild::widget_enabled() const {
	return this->flags & g_widget_enabled;
}

bool guild::has_invite_splash() const {
	return this->flags & g_invite_splash;
}

bool guild::has_vip_regions() const {
	return this->flags & g_vip_regions;
}

bool guild::has_vanity_url() const {
	return this->flags & g_vanity_url;
}

bool guild::is_verified() const {
	return this->flags & g_verified;
}

bool guild::is_partnered() const {
	return this->flags & g_partnered;
}

bool guild::is_community() const {
	return this->flags & g_community;
}

bool guild::has_commerce() const {
	return this->flags & g_commerce;
}

bool guild::has_news() const {
	return this->flags & g_news;
}

bool guild::is_discoverable() const {
	return this->flags & g_discoverable;
}

bool guild::is_featureable() const {
	return this->flags & g_featureable;
}

bool guild::has_animated_icon() const {
	return this->flags & g_animated_icon;
}

bool guild::has_banner() const {
	return this->flags & g_banner;
}

bool guild::is_welcome_screen_enabled() const {
	return this->flags & g_welcome_screen_enabled;
}

bool guild::has_member_verification_gate() const {
	return this->flags & g_member_verification_gate;
}

bool guild::is_preview_enabled() const {
	return this->flags & g_preview_enabled;
}

std::string guild::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	if (!name.empty()) {
		j["name"] = name;
	}
	j["widget_enabled"] = widget_enabled();
	if (afk_channel_id) {
		j["afk_channel_id"] = afk_channel_id;
	}
	if (afk_channel_id) {
		j["afk_timeout"] = afk_timeout;
	}
	if (widget_enabled()) {
		j["widget_channel_id"] = widget_channel_id;
	}
	j["default_message_notifications"] = default_message_notifications;
	j["explicit_content_filter"] = explicit_content_filter;
	j["mfa_level"] = mfa_level;
	if (system_channel_id) {
		j["system_channel_id"] = system_channel_id;
	}
	if (rules_channel_id) {
		j["rules_channel_id"] = rules_channel_id;
	}
	if (!vanity_url_code.empty()) {
		j["vanity_url_code"] = vanity_url_code;
	}
	if (!description.empty()) {
		j["description"] = description;
	}
	return j.dump();
}

guild& guild::fill_from_json(nlohmann::json* d) {
	this->id = SnowflakeNotNull(d, "id");
	if (d->find("unavailable") == d->end() || (*d)["unavailable"].get<bool>() == false) {
		this->name = StringNotNull(d, "name");
		this->icon = StringNotNull(d, "icon");
		this->discovery_splash = StringNotNull(d, "discovery_splash");
		this->owner_id = SnowflakeNotNull(d, "owner_id");
		if (!(*d)["region"].is_null()) {
			auto r = regionmap.find((*d)["region"].get<std::string>());
			if (r != regionmap.end()) {
				this->voice_region = r->second;
			}
		}
		this->flags |= BoolNotNull(d, "large") ? dpp::g_large : 0;
		this->flags |= BoolNotNull(d, "widget_enabled") ? dpp::g_widget_enabled : 0;

		for (auto & feature : (*d)["features"]) {
			auto f = featuremap.find(feature.get<std::string>());
			if (f != featuremap.end()) {
				this->flags |= f->second;
			}
		}
		uint8_t scf = Int8NotNull(d, "system_channel_flags");
		if (scf & 1) {
			this->flags |= dpp::g_no_join_notifications;
		}
		if (scf & 2) {
			this->flags |= dpp::g_no_boost_notifications;
		}

		this->afk_channel_id = SnowflakeNotNull(d, "afk_channel_id");
		this->afk_timeout = Int16NotNull(d, "afk_timeout");
		this->widget_channel_id = SnowflakeNotNull(d, "widget_channel_id");
		this->verification_level = Int8NotNull(d, "verification_level");
		this->default_message_notifications = Int8NotNull(d, "default_message_notifications");
		this->explicit_content_filter = Int8NotNull(d, "explicit_content_filter");
		this->mfa_level = Int8NotNull(d, "mfa_level");
		this->application_id = SnowflakeNotNull(d, "application_id");
		this->system_channel_id = SnowflakeNotNull(d, "system_channel_id");
		this->rules_channel_id = SnowflakeNotNull(d, "rules_channel_id");
		this->member_count = Int32NotNull(d, "member_count");
		this->vanity_url_code = StringNotNull(d, "vanity_url_code");
		this->description = StringNotNull(d, "description");
		this->banner = StringNotNull(d, "banner");
		this->premium_tier = Int8NotNull(d, "premium_tier");
		this->premium_subscription_count = Int16NotNull(d, "premium_subscription_count");
		this->public_updates_channel_id = SnowflakeNotNull(d, "public_updates_channel_id");
		this->max_video_channel_users = Int32NotNull(d, "max_video_channel_users");
	} else {
		this->flags |= dpp::g_unavailable;
	}
	return *this;
}

};

