#pragma once

#include <memory>
#include <string>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/version.hpp>

// Helpers for the libtorrent 2.1 API, where a torrent's immutable info
// section (torrent_info) is separated from everything else in the .torrent
// file (trackers, comment, web seeds - carried by add_torrent_params).
namespace pt
{
namespace BitTorrent
{
    // add_torrent_params::ti is a pointer-to-const, except in libtorrent
    // builds that keep the deprecated API around.
    inline void SetTorrentInfo(libtorrent::add_torrent_params& params, std::shared_ptr<const libtorrent::torrent_info> const& ti)
    {
#if TORRENT_ABI_VERSION < 4
        params.ti = ti ? std::make_shared<libtorrent::torrent_info>(*ti) : nullptr;
#else
        params.ti = ti;
#endif
    }

    // Magnet link for a torrent in the session: info-hashes, name, trackers
    // and web seeds.
    inline std::string MakeMagnetUri(libtorrent::torrent_handle const& handle)
    {
        if (!handle.is_valid()) { return std::string(); }

        libtorrent::add_torrent_params params;
        params.info_hashes = handle.info_hashes();
        params.name = handle.status(libtorrent::torrent_handle::query_name).name;

        for (auto const& tracker : handle.trackers())
        {
            params.trackers.push_back(tracker.url);
            params.tracker_tiers.push_back(tracker.tier);
        }

        for (auto const& seed : handle.url_seeds())
        {
            params.url_seeds.push_back(seed);
        }

        return libtorrent::make_magnet_uri(params);
    }
}
}
