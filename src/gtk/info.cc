// -*-c++-*-

// info.cc
//
//  Copyright 1999-2008 Daniel Burrows
//  Copyright 2008 Obey Arthur Liu
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#include "info.h"
#include "aptitude.h"

#undef OK
#include <gtkmm.h>

#include <cwidget/generic/util/ssprintf.h>

#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/version.h>

#include <generic/apt/apt.h>
#include <generic/util/undo.h>

#include <gtk/gui.h>
#include <gtk/entityview.h>
#include <gtk/packageinformation.h>

namespace gui
{
  namespace
  {
    class VersionEntity : public Entity
    {
    private:
      pkgCache::VerIterator ver;

      const entity_state_info &current_state_columns()
      {
        pkgCache::PkgIterator pkg = ver.ParentPkg();

        if(!ver.end() && ver != pkg.CurrentVer())
          return virtual_columns;

	if((*apt_cache_file)[pkg].NowBroken())
	   return broken_columns;

        switch(pkg->CurrentState)
          {
          case pkgCache::State::NotInstalled:
            return not_installed_columns;
          case pkgCache::State::UnPacked:
            return unpacked_columns;
          case pkgCache::State::HalfConfigured:
            return half_configured_columns;
          case pkgCache::State::HalfInstalled:
            return half_installed_columns;
          case pkgCache::State::ConfigFiles:
            return config_files_columns;
      #ifdef APT_HAS_TRIGGERS
          case pkgCache::State::TriggersAwaited:
            return triggers_awaited_columns;
          case pkgCache::State::TriggersPending:
            return triggers_pending_columns;
      #endif
          case pkgCache::State::Installed:
            return installed_columns;
          default:
            return error_columns;
          }
      }

      // We output a flag/color pair
      // TODO: choose more sensible colors?
      std::pair<const entity_state_info, std::string> action_row_info()
      {
        if(ver.end())
          return std::make_pair(entity_state_info(), "white");

        pkgCache::PkgIterator pkg = ver.ParentPkg();
        aptitudeDepCache::StateCache &state = (*apt_cache_file)[pkg];
        aptitudeDepCache::aptitude_state &estate = (*apt_cache_file)->get_ext_state(pkg);
        pkgCache::VerIterator candver = state.CandidateVerIter(*apt_cache_file);

        if(state.Status!=2 && estate.selection_state == pkgCache::State::Hold && !state.NowBroken())
          return std::make_pair(hold_columns, "gray");
        else if(ver.VerStr() == estate.forbidver)
          return std::make_pair(forbid_columns, "dim gray");
        else if(state.Delete())
          return (state.iFlags&pkgDepCache::Purge)
	    ? std::make_pair(purge_columns,"sky blue")
	    : std::make_pair(remove_columns, "dark turquoise");
        else if(state.InstBroken() && state.InstVerIter(*apt_cache_file) == ver)
          return std::make_pair(broken_columns, "orange red");
        else if(state.NewInstall())
          {
            if(candver==ver)
              return std::make_pair(install_columns, "green");
            else
              return std::make_pair(entity_state_info(), "white");
          }
        else if(state.iFlags & pkgDepCache::ReInstall)
          {
            if(ver.ParentPkg().CurrentVer() == ver)
              return std::make_pair(install_columns, "yellow green");
            else
              return std::make_pair(entity_state_info(),"white");
          }
        else if(state.Upgrade())
          {
            if(ver.ParentPkg().CurrentVer() == ver)
              return std::make_pair(remove_columns, "orange");
            else if(candver == ver)
              return std::make_pair(install_columns, "green yellow");
            else
              return std::make_pair(entity_state_info(), "white");
          }
        else
          return std::make_pair(entity_state_info(), "white");
      }

    public:
      VersionEntity(const pkgCache::VerIterator &_ver)
	: ver(_ver)
      {
      }

      const pkgCache::VerIterator &get_ver() const { return ver; }

      void fill_row(const EntityColumns *columns, Gtk::TreeModel::Row &row)
      {
	using cwidget::util::ssprintf;

	row[columns->EntObject] = this;
        std::pair<entity_state_info, std::string> row_info = action_row_info();
	row[columns->BgSet] = (row_info.second != "white");
	row[columns->BgColor] = row_info.second;

	entity_state_info current_state(current_state_columns());
	row[columns->CurrentStatusIcon] = current_state.get_icon().get_string();
	row[columns->SelectedStatusIcon] = row_info.first.get_icon().get_string();
	row[columns->StatusDescriptionMarkup] =
	  ssprintf("<b>%s:</b> %s\n<b>%s:</b> %s",
		   _("Current status"),
		   _(current_state.get_description().c_str()),
		   _("Selected status"),
		   _(row_info.first.get_description().c_str()));

	row[columns->NameMarkup] = Glib::Markup::escape_text(ver.ParentPkg().Name());
	row[columns->VersionMarkup] = Glib::Markup::escape_text(ver.VerStr());
	row[columns->Name] = ver.ParentPkg().Name();
	row[columns->Version] = ver.VerStr();
      }

      void add_packages(std::set<pkgCache::PkgIterator> &packages)
      {
	packages.insert(ver.ParentPkg());
      }

      void activated(const Gtk::TreeModel::Path &path,
		     const Gtk::TreeViewColumn *column,
		     const EntityView *view)
      {
	InfoTab::show_tab(ver.ParentPkg(), ver);
      }

      void add_actions(std::set<PackagesAction> &actions)
      {
        // This is all highly hackish
        if(ver==ver.ParentPkg().CurrentVer())
          {
          actions.insert(Hold);
          actions.insert(Purge);
            if(!(*apt_cache_file)[ver.ParentPkg()].Keep())
              actions.insert(Keep);

            if((*apt_cache_file)[ver.ParentPkg()].iFlags&pkgDepCache::ReInstall)
              actions.insert(Keep);
            else
              actions.insert(Remove);
          }
        else if(ver==(*apt_cache_file)[ver.ParentPkg()].CandidateVerIter(*apt_cache_file) && (*apt_cache_file)[ver.ParentPkg()].Install())
          actions.insert(Keep);
        else
          {
          if(!(*apt_cache_file)[ver.ParentPkg()].Keep())
            actions.insert(Keep);
          // TODO: what about downgrade ?
          actions.insert(Upgrade);
          }
      }

      void dispatch_action(PackagesAction action, bool first_pass)
      {
        undo_group *undo = new undo_group;
        switch(action)
        {
        case Install:
        case Upgrade:
        case Downgrade:
          (*apt_cache_file)->set_candidate_version(ver, undo);
          (*apt_cache_file)->mark_install(ver.ParentPkg(), !first_pass, false, undo);
          break;
        case Remove:
          (*apt_cache_file)->mark_delete(ver.ParentPkg(), false, false, undo);
          break;
        case Purge:
          (*apt_cache_file)->mark_delete(ver.ParentPkg(), true, false, undo);
          break;
        case Keep:
          (*apt_cache_file)->mark_keep(ver.ParentPkg(), false, false, undo);
          break;
        case Hold:
          (*apt_cache_file)->mark_delete(ver.ParentPkg(), false, (*apt_cache_file)->get_ext_state(ver.ParentPkg()).selection_state!=pkgCache::State::Hold, undo);
          break;
	case MakeAutomatic:
	  (*apt_cache_file)->mark_auto_installed(ver.ParentPkg(), true, undo);
	  break;
	case MakeManual:
	  (*apt_cache_file)->mark_auto_installed(ver.ParentPkg(), false, undo);
	  break;
        default:
          break;
        }
        if(undo->empty())
          delete undo;
        else
          apt_undos->add_item(undo);
      }
    };

    class NotAvailableEntity : public Entity
    {
      Glib::ustring text;
    public:
      NotAvailableEntity(const Glib::ustring &_text) : text(_text) { }

      void fill_row(const EntityColumns *columns, Gtk::TreeModel::Row &row)
      {
	row[columns->EntObject] = EntityRef(this);

	row[columns->BgSet] = true;
	row[columns->BgColor] = "#FFA0A0";
	row[columns->CurrentStatusIcon] = "";
	row[columns->SelectedStatusIcon] = "";
	row[columns->NameMarkup] = text;
	row[columns->VersionMarkup] = "";
	row[columns->Name] = text;
	row[columns->Version];
      }

      void add_packages(std::set<pkgCache::PkgIterator> &packages)
      {
      }

      void activated(const Gtk::TreeModel::Path &path,
		     const Gtk::TreeViewColumn *column,
		     const EntityView *view)
      {
      }

      void add_actions(std::set<PackagesAction> &actions)
      {
      }

      void dispatch_action(PackagesAction action, bool first_pass)
      {
      }
    };

    class DependencyEntity : public HeaderEntity
    {
      pkgCache::DepIterator firstdep;
    public:
      DependencyEntity(pkgCache::DepIterator dep)
	: HeaderEntity(""),firstdep(dep)
      {
	Glib::ustring text = dep.DepType();
	text += ": ";
	do
	  {
	    text += dep.TargetPkg().Name();
	    if((dep->CompareOp & (~pkgCache::Dep::Or)) != pkgCache::Dep::NoOp &&
	       dep.TargetVer() != NULL)
	      {
		text += " (";
		text += dep.CompType();
		text += " ";
		text += dep.TargetVer();
		text += ")";
	      }

	    if(dep->CompareOp & pkgCache::Dep::Or)
	      text += " | ";

	    ++dep;
	  } while(dep->CompareOp & pkgCache::Dep::Or);

	set_text(text);
      }

      bool is_broken()
      {
        pkgCache::DepIterator dep=firstdep;

        // Ok, here's the story: we need to check the DepGInstall dependency flag
        // for the whole `or' group.  HOWEVER, this is only set for the LAST member
        // of an `or' group.  SO, we need to find the last item in the group in order
        // to do this.
        //
        // Alles klar? :)
        while(dep->CompareOp & pkgCache::Dep::Or)
          ++dep;

        // Showing "Replaces" dependencies as broken is weird.
        return ((firstdep.IsCritical() ||
                 firstdep->Type==pkgCache::Dep::Recommends ||
                 firstdep->Type==pkgCache::Dep::Suggests) &&
                !((*apt_cache_file)[dep]&pkgDepCache::DepGInstall));
      }

      void fill_row(const EntityColumns *columns, Gtk::TreeModel::Row &row)
      {
	HeaderEntity::fill_row(columns, row);
	row[columns->SelectedStatusIcon] = Gtk::Stock::YES.id;
	// TODO: Need backend support for detecting changes to
	// dependency state. The state is not updated if
	// the dependency is met after fill_row is executed.
	if(is_broken()) {
	  row[columns->BgSet] = true;
	  row[columns->BgColor] = "orange red";
	}
      }
    };

    class DependencyResolverEntity : public HeaderEntity
    {
      Glib::ustring version_markup;
      Glib::ustring version_text;

    public:
      DependencyResolverEntity(const Glib::ustring &package_text,
			       const Glib::ustring &_version_markup,
			       const Glib::ustring &_version_text)
	: HeaderEntity(package_text), version_markup(_version_markup), version_text(_version_text)
      {
      }

      void fill_row(const EntityColumns *columns, Gtk::TreeModel::Row &row)
      {
	HeaderEntity::fill_row(columns, row);
	row[columns->SelectedStatusIcon] = Gtk::Stock::YES.id;
	row[columns->VersionMarkup] = version_markup;
	row[columns->Version] = version_text;
      }
    };

    // Build a tree of dependencies for the given package version.
    Glib::RefPtr<Gtk::TreeModel> make_depends_tree(const EntityColumns *columns,
						   const pkgCache::VerIterator &ver)
    {
      Glib::RefPtr<Gtk::TreeStore> store = Gtk::TreeStore::create(*columns);

      if(ver.end())
	return store;

      pkgCache::DepIterator dep = ver.DependsList();
      while(!dep.end())
      {
        pkgCache::DepIterator start, end;
        surrounding_or(dep, start, end);
        bool first = true;

        Gtk::TreeModel::iterator tree;
        Gtk::TreeModel::Row row;

        for(pkgCache::DepIterator todisp = start;
	    todisp != end; ++todisp)
        {
          Gtk::TreeModel::iterator tree2;
          Gtk::TreeModel::Row row2;

          const bool is_or_continuation = !first;
          first = false;

          if(!is_or_continuation)
            {
              tree = store->append();
              row = *tree;
	      Entity *ent = new DependencyEntity(todisp);
	      ent->fill_row(columns, row);
            }

          tree2 = store->append(tree->children());
          row2 = *tree2;

	  Glib::ustring version_text, version_markup;

          if(todisp->CompareOp != pkgCache::Dep::NoOp &&
              todisp.TargetVer() != NULL)
          {
            version_markup = Glib::Markup::escape_text(Glib::ustring(todisp.CompType())+" "+Glib::ustring(todisp.TargetVer()));
	    version_text = todisp.TargetVer();
          }
          else
          {
            version_markup = "N/A";
	    version_text = "N/A";
          }

	  (new DependencyResolverEntity(todisp.TargetPkg().Name(),
					version_markup,
					version_text))->fill_row(columns, row2);

          bool resolvable = false;

          // Insert the various resolutions of this dep.  First direct
          // resolutions:
          {
            std::vector<pkgCache::VerIterator> direct_resolutions;
            for(pkgCache::VerIterator ver = todisp.TargetPkg().VersionList();
            !ver.end(); ++ver)
            {
              if(_system->VS->CheckDep(ver.VerStr(), todisp->CompareOp, todisp.TargetVer()))
              {
                resolvable = true;
                direct_resolutions.push_back(ver);
              }
            }

            if(!direct_resolutions.empty())
            {
              for(std::vector<pkgCache::VerIterator>::const_iterator it = direct_resolutions.begin();
              it != direct_resolutions.end(); ++it)
              {
                Gtk::TreeModel::iterator tree3 = store->append(tree2->children());
                Gtk::TreeModel::Row row3 = *tree3;
		(new VersionEntity(*it))->fill_row(columns, row3);
              }
            }
          }

          // Check for resolutions through virtual deps.
          {
            std::vector<pkgCache::VerIterator> virtual_resolutions;

            for(pkgCache::PrvIterator prv = todisp.TargetPkg().ProvidesList();
            !prv.end(); ++prv)
            {
              if(_system->VS->CheckDep(prv.ProvideVersion(), todisp->CompareOp, todisp.TargetVer()))
              {
                resolvable = true;
                virtual_resolutions.push_back(prv.OwnerVer());
              }
            }


            if(!virtual_resolutions.empty())
            {
              for(std::vector<pkgCache::VerIterator>::const_iterator it = virtual_resolutions.begin();
              it != virtual_resolutions.end(); ++it)
              {
                Gtk::TreeModel::iterator tree3 = store->append(tree2->children());
                Gtk::TreeModel::Row row3 = *tree3;
		(new VersionEntity(*it))->fill_row(columns, row3);
              }
            }
          }

          if(!resolvable)
          {
            Gtk::TreeModel::iterator tree3 = store->append(tree2->children());
            Gtk::TreeModel::Row row3 = *tree3;

	    (new NotAvailableEntity(_("Not available")))->fill_row(columns, row3);
          }
        }

	dep = end;
      }

      store->set_sort_column(columns->Version, Gtk::SORT_ASCENDING);
      return store;
    }

    Glib::RefPtr<Gtk::TreeModel> make_version_list(const EntityColumns *columns,
						   const pkgCache::PkgIterator &pkg)
    {
      Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(*columns);

      for (pkgCache::VerIterator ver = pkg.VersionList(); ver.end() == false; ver++)
      {
        Gtk::TreeModel::iterator iter = store->append();
        Gtk::TreeModel::Row row = *iter;

	(new VersionEntity(ver))->fill_row(columns, row);
      }

      // \todo Sort according to version number, not according to
      // version string. (see apt-pkg/version.h)
      store->set_sort_column(columns->Version, Gtk::SORT_ASCENDING);
      return store;
    }
  }

  InfoTab::InfoTab(const Glib::ustring &label)
  : Tab(Info, label, Gnome::Glade::Xml::create(glade_main_file, "main_info_hpaned"), "main_info_hpaned")
  {
    get_xml()->get_widget("main_info_textview", textview);
    get_xml()->get_widget("main_info_notebook", notebook);
    get_widget()->show();
    cache_closed.connect(sigc::mem_fun(*this, &InfoTab::do_cache_closed));
    cache_reloaded.connect(sigc::mem_fun(*this, &InfoTab::do_cache_reloaded));
  }

  void InfoTab::do_cache_closed()
  {
    // The package and version views will handle themselves; we just need to deal
    // with the TextBuffer that shows the current package's state.
    Glib::RefPtr<Gtk::TextBuffer> reloadingBuffer = Gtk::TextBuffer::create();
    reloadingBuffer->insert(reloadingBuffer->end(), _("Please wait; reloading cache..."));
    textview->set_buffer(reloadingBuffer);
  }

  void InfoTab::do_cache_reloaded()
  {
    pkgCache::PkgIterator pkg = (*apt_cache_file)->FindPkg(package_name);
    if(pkg.end())
      {
	tab_del(this);
	return;
      }

    pkgCache::VerIterator found_ver(*apt_cache_file);
    for(pkgCache::VerIterator ver = pkg.VersionList();
	found_ver.end() && !ver.end(); ++ver)
      {
	if(ver.VerStr() == version_name)
	  found_ver = ver;
      }

    if(found_ver.end())
      {
	tab_del(this);
	return;
      }

    disp_package(pkg, found_ver);
  }

  void InfoTab::notebook_switch_handler(GtkNotebookPage * page, guint page_num)
  {
    if (page_num == 1 && !changelog_loaded)
      {
	Glib::RefPtr<Gtk::TextBuffer> text_buffer = Gtk::TextBuffer::create();

	changelog_textview->set_buffer(text_buffer);
	fetch_and_show_changelog(current_version,
				 text_buffer,
				 text_buffer->end());
        changelog_loaded = true;
      }
    else if (page_num == 2 && !filesview_loaded)
      {
        filesview->load_version(current_version);
        filesview_loaded = true;
      }
  }

  void InfoTab::disp_package(pkgCache::PkgIterator pkg, pkgCache::VerIterator ver)
  {
    changelog_loaded = false;
    filesview_loaded = false;
    current_version = ver;

    package_name = pkg.end() ? "" : pkg.Name();
    version_name = ver.end() ? "" : ver.VerStr();

    Glib::RefPtr<Gtk::TextBuffer> textBuffer = Gtk::TextBuffer::create();
    set_label("Info on " + Glib::ustring(pkg.Name()));

    PackageInformation info(pkg, ver);

    Glib::RefPtr<Gtk::TextBuffer::Tag> nameTag = textBuffer->create_tag();
    nameTag->property_size() = 20 * Pango::SCALE;

    Glib::RefPtr<Gtk::TextBuffer::Tag> fieldNameTag = textBuffer->create_tag();
    fieldNameTag->property_weight() = 2 * Pango::SCALE;

    textBuffer->insert_with_tag(textBuffer->end(),
        info.Name(),
        nameTag);

    textBuffer->insert_with_tag(textBuffer->end(),
        " ",
        nameTag);


    textBuffer->insert_with_tag(textBuffer->end(),
        info.Version(),
        nameTag);

    textBuffer->insert(textBuffer->end(), "\n");
    textBuffer->insert(textBuffer->end(), info.ShortDescription());
    textBuffer->insert(textBuffer->end(), "\n");

    // TODO: insert a horizontal rule here (how?)

    textBuffer->insert(textBuffer->end(), "\n");

    //pkgRecords::Parser &rec=apt_package_records->Lookup(ver.FileList());

    textBuffer->insert(textBuffer->end(), "\n");

    std::wstring longdesc = get_long_description(ver, apt_package_records);

    textBuffer->insert_with_tag(textBuffer->end(), _("Description: "), fieldNameTag);

    textBuffer->insert(textBuffer->end(), info.LongDescription());

    textview->set_buffer(textBuffer);

    using cwidget::util::ref_ptr;
    pVersionsView = ref_ptr<EntityView>(new EntityView(get_xml(),
						       "main_info_versionsview",
						       _("Package information: version list")));
    pVersionsView->set_model(make_version_list(pVersionsView->get_columns(), pkg));
    pVersionsView->get_name_column()->set_fixed_width(154);
    pVersionsView->get_automatically_installed_column()->set_visible(false);
    pVersionsView->get_treeview()->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
    {
      Gtk::TreeModel::Children entries = pVersionsView->get_treeview()->get_model()->children();
      for(Gtk::TreeModel::Children::const_iterator it = entries.begin();
	  it != entries.end(); ++it)
	{
	  using cwidget::util::ref_ptr;
	  ref_ptr<Entity> ent = (*it)[pVersionsView->get_columns()->EntObject];
	  ref_ptr<VersionEntity> ver_ent = ent.dyn_downcast<VersionEntity>();
          if(ver_ent.valid() && ver == ver_ent->get_ver())
	    pVersionsView->get_treeview()->get_selection()->select(it);
	}
    }

    pDependsView = ref_ptr<EntityView>(new EntityView(get_xml(), "main_info_dependsview",
						      _("Package information: dependency list")));
    pDependsView->set_model(make_depends_tree(pDependsView->get_columns(), ver));
    pDependsView->get_expander_column()->set_fixed_width(48);
    pDependsView->get_name_column()->set_fixed_width(280);
    pDependsView->get_automatically_installed_column()->set_visible(false);
    Gtk::TreeModel::Children dependsChildren = pDependsView->get_treeview()->get_model()->children();
    for(Gtk::TreeModel::iterator it = dependsChildren.begin();
	it != dependsChildren.end(); ++it)
      {
	// Expand all the top-level entries, which we magically know
	// contain the individual components of a dependency.
	Gtk::TreeModel::Path path = pDependsView->get_treeview()->get_model()->get_path(it);
	pDependsView->get_treeview()->expand_row(path, false);
      }

    get_xml()->get_widget("main_info_changelogview", changelog_textview);

    filesview = new FilesView(get_xml(), "main_info_filesview");

    notebook->signal_switch_page().connect(sigc::mem_fun(*this, &InfoTab::notebook_switch_handler));
  }

  void InfoTab::show_tab(const pkgCache::PkgIterator &pkg,
			 const pkgCache::VerIterator &ver)
  {
    InfoTab * infotab = new InfoTab(_("Info"));
    tab_add(infotab);
    infotab->disp_package(pkg, ver);
  }
}
