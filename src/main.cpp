#include <ctime>
#include <cmath>
#include <format>
#include <future>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <fmt/format.h>

#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/cocos/CCDirector.h>
#include <Geode/utils/VMTHookManager.hpp>
#include <Geode/cocos/label_nodes/CCLabelBMFont.h>

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/PlatformToolbox.hpp>

using namespace geode::prelude;

auto director = CCDirector::sharedDirector();
bool answered = true;

std::string polynomial_names[5] = {"[ERROR 0]", "[ERROR 1]", "Quadratic", "Cubic", "Quartic"};
int polynomial_degree = 2;
int frequency = 20;
int range = 8;

$on_mod(Loaded) {
    polynomial_degree = Mod::get()->getSettingValue<int>("polynomial-degree");
    listenForSettingChanges<int>("polynomial-degree", [](int value) {
        polynomial_degree = value;
    });
    frequency = Mod::get()->getSettingValue<int>("frequency");
    listenForSettingChanges<int>("frequency", [](int value) {
        frequency = value;
    });
    range = Mod::get()->getSettingValue<int>("range");
    listenForSettingChanges<int>("range", [](int value) {
        range = value;
    });
}

std::string filterNumbers(std::string const& text) {
    std::string result = "";

    for (int i = 0; i < text.size(); i++) {
        char c = text[i];

        if (std::isdigit(c)) {
            result += c;
        }
        else if (c == '-' && i == 0) {
            result += c;
        }
    }

    return result;
}

class PolynomialPopup : public geode::Popup {
protected:
    int m_a;
    int m_b;
    int m_c;
    int m_d;
    int m_degree;

    TextInput* m_input_a;
    TextInput* m_input_b;
    TextInput* m_input_c;
    TextInput* m_input_d;

    bool init(int degree, int a, int b, int c = 0, int d = 0) {
        if (!Popup::init(480.f, 160.f))
            return false;

        m_a = a;
        m_b = b;
        m_c = c;
        m_d = d;
        m_degree = degree;

        this->m_closeBtn->setVisible(false);

        this->setTitle(fmt::format(
            "Find all {} roots of the following {} to play the next attempt!",
            degree,
            polynomial_names[degree]
        ));

        CCLabelBMFont* label;
        if (degree == 2) {
            int p_a = a + b;
            int p_b = a * b;
            label = CCLabelBMFont::create(fmt::format(
                "x^2{}{}{}{}{} = 0",
                p_a == 0 ? "" : (p_a >= 0 ? " + " : " - "),
                p_a == 0 ? "" : (abs(p_a) == 1 ? "" : std::to_string(abs(p_a))),
                p_a == 0 ? "" : "x",
                p_b == 0 ? "" : (p_b >= 0 ? " + " : " - "),
                p_b == 0 ? "" : std::to_string(abs(p_b))
            ).c_str(), "bigFont.fnt");
        } else if (degree == 3) {
            int p_a = a + b + c;
            int p_b = (a*b) + (a*c) + (b*c);
            int p_c = a * b * c;
            label = CCLabelBMFont::create(fmt::format(
                "x^3{}{}{}{}{}{}{}{} = 0",
                p_a == 0 ? "" : (p_a >= 0 ?  " + " : " - "),
                p_a == 0 ? "" : (abs(p_a) == 1 ? "" : std::to_string(abs(p_a))),
                p_a == 0 ? "" : "x^2",
                p_b == 0 ? "" : (p_b >= 0 ?  " + " : " - "),
                p_b == 0 ? "" : (abs(p_b) == 1 ? "" : std::to_string(abs(p_b))),
                p_b == 0 ? "" : "x",
                p_c == 0 ? "" : (p_c >= 0 ?  " + " : " - "),
                p_c == 0 ? "" : std::to_string(abs(p_c))
            ).c_str(), "bigFont.fnt");
        } else {
            int p_a = a + b + c + d;
            int p_b = (a*b) + (a*c) + (a*d) + (b*c) + (b*d) + (c*d);
            int p_c = (a*b*c) + (a*b*d) + (a*c*d) + (b*c*d);
            int p_d = a * b * c * d;
            label = CCLabelBMFont::create(fmt::format(
                "x^4{}{}{}{}{}{}{}{}{}{}{} = 0",
                p_a == 0 ? "" : (p_a >= 0 ?  " + " : " - "),
                p_a == 0 ? "" : (abs(p_a) == 1 ? "" : std::to_string(abs(p_a))),
                p_a == 0 ? "" : "x^3",
                p_b == 0 ? "" : (p_b >= 0 ?  " + " : " - "),
                p_b == 0 ? "" : (abs(p_b) == 1 ? "" : std::to_string(abs(p_b))),
                p_b == 0 ? "" : "x^2",
                p_c == 0 ? "" : (p_c >= 0 ?  " + " : " - "),
                p_c == 0 ? "" : (abs(p_c) == 1 ? "" : std::to_string(abs(p_c))),
                p_c == 0 ? "" : "x",
                p_d == 0 ? "" : (p_d >= 0 ?  " + " : " - "),
                p_d == 0 ? "" : std::to_string(abs(p_d))
            ).c_str(), "bigFont.fnt");
            label->setScale(0.6);
        }
        label->setPosition(240.f, 100.f);
        m_mainLayer->addChild(label);
        
        if (degree == 2) {
            m_input_a = TextInput::create(100.f, "Root 1");
            m_input_a->setPosition({185.f, 60.f});
            m_input_a->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_a->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_a);
            m_input_b = TextInput::create(100.f, "Root 2");
            m_input_b->setPosition({295.f, 60.f});
            m_input_b->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_b->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_b);
        } else if (degree == 3) {
            m_input_a = TextInput::create(100.f, "Root 1");
            m_input_a->setPosition({130.f, 60.f});
            m_input_a->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_a->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_a);
            m_input_b = TextInput::create(100.f, "Root 2");
            m_input_b->setPosition({240.f, 60.f});
            m_input_b->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_b->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_b);
            m_input_c = TextInput::create(100.f, "Root 3");
            m_input_c->setPosition({350.f, 60.f});
            m_input_c->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_c->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_c);
        } else {
            m_input_a = TextInput::create(100.f, "Root 1");
            m_input_a->setPosition({75.f, 60.f});
            m_input_a->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_a->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_a);
            m_input_b = TextInput::create(100.f, "Root 2");
            m_input_b->setPosition({185.f, 60.f});
            m_input_b->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_b->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_b);
            m_input_c = TextInput::create(100.f, "Root 3");
            m_input_c->setPosition({295.f, 60.f});
            m_input_c->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_c->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_c);
            m_input_d = TextInput::create(100.f, "Root 4");
            m_input_d->setPosition({405.f, 60.f});
            m_input_d->setCallback([this](const std::string& text) {
                auto filtered = filterNumbers(text);
                if (text != filtered) {
                    m_input_d->setString(filtered);
                }
            });
            m_mainLayer->addChild(m_input_d);
        }

        auto menu = CCMenu::create(); // fsr i gotta do this
        menu->setPosition({0.f, 0.f});

        auto button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Submit"),
            this,
            menu_selector(PolynomialPopup::onSubmit)
        );
        button->setPosition(240.f, 25.f);
        menu->addChild(button);
        m_mainLayer->addChild(menu);

        return true;
    } 

    void onSubmit(CCObject*) {
        bool correct = true;
        // turns out i cant use trycatch. whoops!
        // sorry about that.            - sneefum
        Result<int, geode::utils::NumParseError> a_res = numFromString<int>(m_input_a->getString()); 
        Result<int, geode::utils::NumParseError> b_res = numFromString<int>(m_input_b->getString());
        Result<int, geode::utils::NumParseError> c_res = numFromString<int>("0"); // this is the best way i could find to initialize these
        if (m_degree >= 3) {
            c_res = numFromString<int>(m_input_c->getString());
        }
        Result<int, geode::utils::NumParseError> d_res = numFromString<int>("0"); // im so sorry
        if (m_degree >= 4) {
            d_res = numFromString<int>(m_input_d->getString());
        }

        if (!a_res.isOk() || !b_res.isOk() || !c_res.isOk() || !d_res.isOk())
            correct = false;

        if (correct) {
            int a = 0 - (a_res.unwrap());
            int b = 0 - (b_res.unwrap());
            int c = 0 - (c_res.unwrap());
            int d = 0 - (d_res.unwrap());
            
            std::array<int, 4> values{};
            std::array<int, 4> expected{};
            if (m_degree == 2) {
                values = std::array{a, b, 0, 0};
                expected = std::array{m_a, m_b, 0, 0};
            } else if (m_degree == 3) {
                values = std::array{a, b, c, 0};
                expected = std::array{m_a, m_b, m_c, 0};
            } else {
                values = std::array{a, b, c, d};
                expected = std::array{m_a, m_b, m_c, m_d};
            }

            std::sort(values.begin(), values.end());
            std::sort(expected.begin(), expected.end());

            int size = m_degree;

            for (int i = 0; i < size; i++) {
                if (values[i] != expected[i])
                    correct = false;
            }
        }
        
        if (correct) {
            answered = true;
            this->onClose(nullptr);
            // cursor function has been moved here
            #if !defined(GEODE_IS_IOS) && !defined(GEODE_IS_ANDROID32) && !defined(GEODE_IS_ANDROID64)
            PlatformToolbox::hideCursor();
            #endif
            // 0.75s grace period to pause and exit
            std::thread([]() {
                std::this_thread::sleep_for(std::chrono::duration<double>(0.75));
                PlayLayer::get()->resetLevel();
                return;
            }).detach();
        } else {
            m_input_a->setString("");
            m_input_b->setString("");
            if (m_degree >= 3)
                m_input_c->setString("");
            if (m_degree >= 4)
                m_input_d->setString("");
        }
        return;
    }

    void keyBackClicked() override {} // no esc key for you!! :3

public:
    static PolynomialPopup* create(int degree, int a, int b, int c = 0, int d = 0) {
        auto ret = new PolynomialPopup();
        if (ret->init(degree, a, b, c, d)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    } 
};

int a, b, c, d;
int deaths = 0;
bool wasDeadLastUpdate = false;

class $modify(MyPlayLayer, PlayLayer) {
    void postUpdate(float dt) {
        auto player = GJBaseGameLayer::m_player1;

        if (answered && player && player->m_isDead && !wasDeadLastUpdate) {
            deaths++;
            if (deaths % frequency == 0) {
                a = (rand() % ((range * 2) + 1)) - range;
                b = (rand() % ((range * 2) + 1)) - range;
                c = (rand() % ((range * 2) + 1)) - range;
                d = (rand() % ((range * 2) + 1)) - range;
                answered = false;
                #if !defined(GEODE_IS_IOS) && !defined(GEODE_IS_ANDROID32) && !defined(GEODE_IS_ANDROID64) // phones dont have cursors
                PlatformToolbox::showCursor();
                #endif
                PolynomialPopup::create(polynomial_degree, a, b, c, d)->show();
            }
        }

        wasDeadLastUpdate = player->m_isDead;

        PlayLayer::postUpdate(dt);
    }

    void resetLevel() {
        if (!answered) {
            return;
        } else {
            PlayLayer::resetLevel();
        }
    }
};

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        if (!answered) {
            return;
        } else {
            GJBaseGameLayer::update(dt);
        }
    }
    void resetPlayer() {
        if (!answered) {
            return;
        } else {
            GJBaseGameLayer::resetPlayer();
        }
    }
};

class $modify(MyPlayerObject, PlayerObject) {
    void update(float dt) {
        if (!answered) {
            return;
        } else {
            PlayerObject::update(dt);
        }
    }
};
