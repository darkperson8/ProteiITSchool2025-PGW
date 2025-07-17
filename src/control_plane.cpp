#include <control_plane.h>

// Найти PDN Connection по CP-TEID
std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    auto it = _pdns.find(cp_teid);
    return it != _pdns.end() ? it->second : nullptr;
}

// Найти PDN Connection по IP-адресу UE
std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    auto it = _pdns_by_ue_ip_addr.find(ip);
    return it != _pdns_by_ue_ip_addr.end() ? it->second : nullptr;
}

// Найти Bearer по DP-TEID
std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    auto it = _bearers.find(dp_teid);
    return it != _bearers.end() ? it->second : nullptr;
}

// Создать PDN Connection для заданного APN
std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn, boost::asio::ip::address_v4 sgw_addr,
                                                                      uint32_t sgw_cp_teid) {
    auto apn_it = _apns.find(apn);
    if (apn_it == _apns.end()) throw std::runtime_error("APN не найден");
    auto apn_gw = apn_it->second;
    // Для простоты назначаем IP UE равным APN Gateway
    auto ue_ip = apn_gw;
    auto pdn = pdn_connection::create(sgw_cp_teid, apn_gw, ue_ip);
    pdn->set_sgw_addr(sgw_addr);
    pdn->set_sgw_cp_teid(sgw_cp_teid);
    _pdns[sgw_cp_teid] = pdn;
    _pdns_by_ue_ip_addr[ue_ip] = pdn;
    return pdn;
}

// Удалить PDN Connection и связанные сессии
void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    auto it = _pdns.find(cp_teid);
    if (it == _pdns.end()) return;
    auto pdn = it->second;
    // Удалить все Bearer, связанные с этим PDN
    for (auto &b_pair : _bearers) {
        if (b_pair.second->get_pdn_connection() == pdn) {
            pdn->remove_bearer(b_pair.first);
        }
    }
    // Удалить их из глобальной карты Bearer
    for (auto it2 = _bearers.begin(); it2 != _bearers.end(); ) {
        if (it2->second->get_pdn_connection() == pdn) it2 = _bearers.erase(it2);
        else ++it2;
    }
    _pdns_by_ue_ip_addr.erase(pdn->get_ue_ip_addr());
    _pdns.erase(it);
}

// Создать новый Bearer в рамках PDN Connection
std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {
    if (!pdn) throw std::runtime_error("PDN Connection пустой");
    uint32_t dp_teid = sgw_teid; // используем тот же TEID для data-plane
    auto b = std::make_shared<bearer>(dp_teid, *pdn);
    b->set_sgw_dp_teid(sgw_teid);
    pdn->add_bearer(b);
    _bearers[dp_teid] = b;
    return b;
}

// Удалить Bearer по DP-TEID
void control_plane::delete_bearer(uint32_t dp_teid) {
    auto it = _bearers.find(dp_teid);
    if (it == _bearers.end()) return;
    auto pdn = it->second->get_pdn_connection();
    pdn->remove_bearer(dp_teid);
    _bearers.erase(it);
}

// Зарегистрировать APN и его шлюз
void control_plane::add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway) {
    _apns.emplace(std::move(apn_name), apn_gateway);
}