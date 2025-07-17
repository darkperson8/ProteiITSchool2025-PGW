#include <data_plane.h>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

// Обработка uplink-трафика: от UE к APN
void data_plane::handle_uplink(uint32_t dp_teid, Packet &&packet) {
    auto b = _control_plane.find_bearer_by_dp_teid(dp_teid);
    if (!b) return; // неизвестный Bearer
    auto pdn = b->get_pdn_connection();
    auto apn_gw = pdn->get_apn_gw();
    forward_packet_to_apn(apn_gw, std::move(packet));
}

// Обработка downlink-трафика: от APN к UE
void data_plane::handle_downlink(const boost::asio::ip::address_v4 &ue_ip, Packet &&packet) {
    auto pdn = _control_plane.find_pdn_by_ip_address(ue_ip);
    if (!pdn) return; // неизвестный UE
    auto sgw_addr = pdn->get_sgw_address();
    auto default_b = pdn->get_default_bearer();
    if (!default_b) return;
    uint32_t sgw_dp_teid = default_b->get_sgw_dp_teid();
    forward_packet_to_sgw(sgw_addr, sgw_dp_teid, std::move(packet));
}
