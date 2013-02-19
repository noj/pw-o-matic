#include "pwgen.hpp"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <set>
#include <memory>

#include <gtkmm.h>

namespace
{
    class Completions {
    public:
        Completions(const std::string& filename)
            : m_filename(filename)
        {
            load();
        }

        void add(const std::string& completion)
        {
            m_completions.insert(completion);
        }

        const std::set<std::string>& completions() const { return m_completions; }

        void store()
        {
            if(m_completions.empty())
                return;

            std::ofstream ofs(m_filename.c_str());
            if(!ofs)
                throw std::runtime_error("Failed to open " + m_filename + " for writing");

            for(auto& completion : m_completions)
                ofs << completion << '\n';

            std::cout << "wrote " << m_completions.size() << " completions" << std::endl;
        }

    private:

        void load()
        {
            std::ifstream ifs(m_filename.c_str());
            if(ifs)
            {
                std::set<std::string> res;

                std::string line;
                while(std::getline(ifs, line))
                    res.insert(line);

                std::cout << "loaded " << res.size() << " completions" << std::endl;

                res.swap(m_completions);
            }
        }

        std::string           m_filename;
        std::set<std::string> m_completions;
    };

    class PwApp {
    public:

        PwApp(Glib::RefPtr<Gtk::Application>& app, const std::string& completions_file)
            : m_app(app)
            , m_builder(Gtk::Builder::create_from_file("gui.glade"))
            , m_completions(completions_file)
            , m_copy_btn(nullptr)
            , m_pw_entry(nullptr)
            , m_site_entry(nullptr)
        {
            setup_dialog();
        }

        int run()
        {
            return m_app->run(*m_dialog);
        }

    private:

        class ModelColumns : public Gtk::TreeModel::ColumnRecord {
        public:
            ModelColumns()
            {
                add(m_col_id);
                add(m_col_name);
            }

            Gtk::TreeModelColumn<unsigned int> m_col_id;
            Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        };

        ModelColumns m_columns;

        template <typename Widget>
        Widget * require_widget(const char * id)
        {
            Widget * widget = nullptr;
            m_builder->get_widget(id, widget);
            if(!widget)
                throw std::runtime_error(std::string("Failed to get widget ") + id);

            return widget;
        }

        void setup_dialog()
        {
            m_dialog = require_widget<Gtk::Dialog>("main_dialog");

            require_widget<Gtk::Button>("ok-button")
                ->signal_clicked().connect([=]{ m_app->quit(); });

            m_copy_btn = require_widget<Gtk::Button>("copy-to-clipboard");
            m_copy_btn->signal_clicked().connect(sigc::mem_fun(this, &PwApp::on_copy_to_clipboard));

            m_pw_entry = require_widget<Gtk::Entry>("master-entry");
            m_pw_entry->signal_changed().connect(sigc::mem_fun(this, &PwApp::maybe_calculate_password));

            m_site_entry = require_widget<Gtk::Entry>("site-entry");
            m_site_entry->signal_changed().connect(sigc::mem_fun(this, &PwApp::maybe_calculate_password));

            m_pass_entry = require_widget<Gtk::Entry>("password-entry");
            m_pass_entry->set_visibility(false);

            auto toggler = require_widget<Gtk::CheckButton>("show-generated-password");
            toggler->signal_toggled().connect([=]
                                              { m_pass_entry->set_visibility(!m_pass_entry->get_visibility()); });

            // Setup completion
            auto completion = Gtk::EntryCompletion::create();
            m_site_entry->set_completion(completion);

            auto compl_model = Gtk::ListStore::create(m_columns);
            completion->set_model(compl_model);

            size_t id = 1;
            for(auto comp : m_completions.completions()) {
                Gtk::TreeModel::Row row = *(compl_model->append());
                row[m_columns.m_col_id] = id++;
                row[m_columns.m_col_name] = comp;
            }

            completion->set_text_column(m_columns.m_col_name);

            maybe_calculate_password();
        }

        void on_copy_to_clipboard()
        {
            if(m_pass_entry->get_text_length() != 0)
            {
                // We only store the completion here, if the user actually
                // copied the generated password to the clipboard, that is to
                // ensure that we don't store loads of junk.
                m_completions.add(m_site_entry->get_text());
                m_completions.store();

                Gtk::Clipboard::get()->set({ Gtk::TargetEntry("UTF8_STRING") },
                                           sigc::mem_fun(this, &PwApp::on_clipboard_get),
                                           sigc::mem_fun(this, &PwApp::on_clipboard_clear));

            }
        }

        void on_clipboard_get(Gtk::SelectionData& selection_data, guint info)
        {
            if(selection_data.get_target() == "UTF8_STRING")
                selection_data.set(selection_data.get_target(), m_pass_entry->get_text());
        }

        void on_clipboard_clear()
        {
            // Currently, we do nothing
        }

        void maybe_calculate_password()
        {
            if(required_fields_set())
            {
                auto master = m_pw_entry->get_text();
                auto site = m_site_entry->get_text();

                auto pass = gen_password(site, master);
                m_pass_entry->set_text(pass);

                m_copy_btn->set_sensitive(true);
            }
            else
            {
                m_copy_btn->set_sensitive(false);
                m_pass_entry->set_text("");
            }
        }

        bool required_fields_set() const
        {
            if(m_pw_entry && m_site_entry)
            {
                return m_pw_entry->get_text_length() != 0
                    && m_site_entry->get_text_length() != 0;
            }

            return false;
        }

        Glib::RefPtr<Gtk::Application> m_app;
        Glib::RefPtr<Gtk::Builder>     m_builder;
        Completions                    m_completions;

        Gtk::Dialog * m_dialog;
        Gtk::Button * m_copy_btn;
        Gtk::Entry * m_pw_entry;
        Gtk::Entry * m_site_entry;
        Gtk::Entry * m_pass_entry;
    };
}

int main(int argc, char ** argv)
{
    try {
        auto app = Gtk::Application::create(argc, argv);

        const char * home_dir = std::getenv("HOME");
        if(!home_dir)
            throw std::runtime_error("Failed to get HOME env variable");

        PwApp pw(app, std::string(home_dir) + "/.gpwcomps");

        return pw.run();

    } catch(const Glib::Exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
