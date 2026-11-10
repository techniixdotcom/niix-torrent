#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <libtorrent/bitfield.hpp>
#include <libtorrent/fwd.hpp>

namespace pt::UI::Widgets
{
    // A strip showing which pieces of a torrent have been downloaded. Each
    // pixel column is shaded by the share of its pieces that are complete, so
    // the map stays meaningful for torrents with more pieces than pixels.
    class PieceProgressBar : public wxPanel
    {
    public:
        PieceProgressBar(wxWindow* parent, wxWindowID id, libtorrent::typed_bitfield<libtorrent::piece_index_t> field = {});
        void UpdateBitfield(libtorrent::typed_bitfield<libtorrent::piece_index_t> const& field);

    private:
        void OnPaint(wxPaintEvent&);

        libtorrent::typed_bitfield<libtorrent::piece_index_t> m_bitfield;
    };
}
