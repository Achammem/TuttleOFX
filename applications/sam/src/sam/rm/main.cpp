#include <sam/common/utility.hpp>
#include <sam/common/color.hpp>
#include <sam/common/options.hpp>

#include <tuttle/common/exceptions.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#include <sequence/parser/Browser.h>
#include <sequence/DisplayUtils.h>

#include <algorithm>
#include <iostream>
#include <iterator>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace bal = boost::algorithm;
namespace ttl = tuttle::common;
using namespace tuttle::common;

bool    enableColor    = false;
bool    verbose        = false;
bool    selectRange    = false;
ssize_t firstImage     = 0;
ssize_t lastImage      = 0;

sam::Color _color;

// A helper function to simplify the main part.
template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
	copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " "));
	return os;
}
/*
void removeSequence( const ttl::Sequence& s )
{
	std::ssize_t first;
	std::ssize_t last;
	if( selectRange )
	{
		first = std::max( s.getFirstTime(), firstImage );
		last  = std::min( s.getLastTime(), lastImage );
	}
	else
	{
		first = s.getFirstTime();
		last  = s.getLastTime();
	}
//	TUTTLE_TCOUT( "remove sequence." );
//	TUTTLE_TCOUT("remove from " << first << " to " << last);

	for( ttl::Sequence::Time t = first; t <= last; t += s.getStep() )
	{
		bfs::path sFile = s.getAbsoluteFilenameAt(t);
		if( !bfs::exists( sFile ) )
		{
			TUTTLE_CERR("Could not remove (file not exist): " << _color._red << sFile.string() << _color._std );
		}
		else
		{
			if(verbose)
			{
				TUTTLE_COUT("remove: " << _color._folder << sFile.string() << _color._std );
			}
			try
			{
				bfs::remove( sFile );
			}
			catch(const boost::filesystem::filesystem_error& e)
			{
//			   if( e.code() == boost::system::errc::permission_denied )
//				   "permission denied"
				TUTTLE_CERR( "sam-rm: Error:\t\n" << e.what() );
				/// @todo cin
//				TUTTLE_COUT( "sam-rm: Continue ? (Yes/No/Yes for All/No for All)" );
				
			}
		}
	}
}

void removeFileObject( std::list<boost::shared_ptr<ttl::FileObject> > &listing, std::vector<boost::filesystem::path> &notRemoved )
{
//	TUTTLE_TCOUT( "removeFileObject." );
	BOOST_FOREACH( const std::list< boost::shared_ptr<ttl::FileObject> >::value_type & s, listing )
	{
		if( !(s->getMaskType () == ttl::eMaskTypeDirectory))
		{
			if(verbose)
				TUTTLE_COUT( "remove: " << *s );
			if( s->getMaskType () == ttl::eMaskTypeSequence )
				removeSequence( (ttl::Sequence&) *s );
			else
			{
				std::vector<bfs::path> paths = s->getFiles();
				for(unsigned int i=0; i<paths.size(); i++)
					bfs::remove( paths.at(i) );
			}
		}
		else // is a directory
		{
			std::vector<boost::filesystem::path> paths = s->getFiles();
			for(unsigned int i=0; i<paths.size(); i++)
			{
				if( bfs::is_empty( paths.at(i) ) )
				{
					if(verbose)
						TUTTLE_COUT( "remove: " << *s );
					bfs::remove( paths.at(i) );
				}
				else
				{
					notRemoved.push_back( paths.at(i) );
				}
			}
		}
	}
}*/

void removeFiles( std::vector<boost::filesystem::path> &listing )
{
	std::sort(listing.begin(), listing.end());
	std::reverse(listing.begin(), listing.end());
	BOOST_FOREACH( const boost::filesystem::path& paths, listing )
	{
		if(bfs::is_empty(paths))
		{
			if(verbose)
			{
				TUTTLE_COUT( "remove: " << _color._folder << paths << _color._std );
			}
			bfs::remove(paths);
		}
		else
		{
			TUTTLE_CERR( "could not remove " << _color._error << paths << _color._std );
		}
	}
}


int main( int argc, char** argv )
{
	using namespace tuttle::common;
	using namespace sam;

	bool recursiveListing    = false;
	bool verbose             = false;
	bool removeUnitFile      = false;
	bool removeFolder        = false;
	bool removeSequences     = true;
	bool removeDotFile       = false; // file starting with a dot (.filename)
	bool printAbsolutePath   = false;
	std::vector<std::string> paths;
	std::vector<std::string> filters;

	// Declare the supported options.
	bpo::options_description mainOptions;
	mainOptions.add_options()
			( kAllOptionString         , kAllOptionMessage )
			( kDirectoriesOptionString , kDirectoriesOptionMessage )
			( kExpressionOptionString  , bpo::value<std::string>(), kExpressionOptionMessage )
			( kFilesOptionString       , kFilesOptionMessage )
			( kHelpOptionString        , kHelpOptionMessage )
			( kIgnoreOptionString      , kIgnoreOptionMessage )
			( kPathOptionString        , kPathOptionMessage )
			( kRecursiveOptionString   , kRecursiveOptionMessage )
			( kVerboseOptionString     , kVerboseOptionMessage )
			( kColorOptionString       , kColorOptionMessage )
			( kFirstImageOptionString  , bpo::value<std::ssize_t>(), kFirstImageOptionMessage )
			( kLastImageOptionString   , bpo::value<std::ssize_t>(), kLastImageOptionMessage )
			( kFullRMPathOptionString  , kFullRMPathOptionMessage )
			( kBriefOptionString       , kBriefOptionMessage )
			;
	
	// describe hidden options
	bpo::options_description hidden;
	hidden.add_options()
			( kInputDirOptionString, bpo::value< std::vector<std::string> >(), kInputDirOptionMessage )
			( kEnableColorOptionString, bpo::value<std::string>(), kEnableColorOptionMessage )
			;
	
	// define default options 
	bpo::positional_options_description pod;
	pod.add( kInputDirOptionString, -1 );
	
	bpo::options_description cmdline_options;
	cmdline_options.add( mainOptions ).add( hidden );

	bpo::positional_options_description pd;
	pd.add( "", -1 );
	
	//parse the command line, and put the result in vm
	bpo::variables_map vm;

	bpo::notify( vm );

	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(pod).run(), vm);

		// get environment options and parse them
		if( const char* env_rm_options = std::getenv("SAM_RM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_rm_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
		if( const char* env_rm_options = std::getenv("SAM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_rm_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
	}
	catch( const bpo::error& e )
	{
		TUTTLE_COUT( "sam-rm: command line error: " << e.what() );
		exit( -2 );
	}
	catch( ... )
	{
		TUTTLE_COUT( "sam-rm: unknown error in command line." );
		exit( -2 );
	}

	if ( vm.count( kColorOptionLongName ) )
	{
		enableColor = true;
	}
	if ( vm.count( kEnableColorOptionLongName ) )
	{
		const std::string str = vm[kEnableColorOptionLongName].as<std::string>();
		enableColor = string_to_boolean( str );
	}

	if( enableColor )
	{
		_color.enable();
	}

	if( vm.count( kHelpOptionLongName ) )
	{
		TUTTLE_COUT( _color._blue  << "TuttleOFX project [http://sites.google.com/site/tuttleofx]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "NAME" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-rm - remove file sequences" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "SYNOPSIS" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-rm [options] [sequence_pattern]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "DESCRIPTION" << _color._std << std::endl );
		TUTTLE_COUT( "Remove sequence of files, and could remove trees (folder, files and sequences)." << std::endl );
		TUTTLE_COUT( _color._blue  << "OPTIONS" << _color._std << std::endl );
		TUTTLE_COUT( mainOptions );

		TUTTLE_COUT( _color._blue << "EXAMPLES" << _color._std << std::left );
		SAM_EXAMPLE_TITLE_COUT( "Sequence possible definitions: " );
		SAM_EXAMPLE_LINE_COUT ( "Auto-detect padding : ", "seq.@.jpg" );
		SAM_EXAMPLE_LINE_COUT ( "Padding of 8 (usual style): ", "seq.########.jpg" );
		SAM_EXAMPLE_LINE_COUT ( "Padding of 8 (printf style): ", "seq.%08d.jpg" );
		SAM_EXAMPLE_TITLE_COUT( "Delete: " );
		SAM_EXAMPLE_LINE_COUT ( "A sequence:", "sam-rm /path/to/sequence/seq.@.jpg" );
		SAM_EXAMPLE_LINE_COUT ( "Sequences in a directory:", "sam-rm /path/to/sequence/" );

		return 0;
	}

	if( vm.count( kBriefOptionLongName ) )
	{
		TUTTLE_COUT( _color._green << "remove file sequences" << _color._std);
		return 0;
	}

	if(vm.count( kExpressionOptionLongName ) )
	{
		bal::split( filters, vm["expression"].as<std::string>(), bal::is_any_of( "," ) );
	}

	if( vm.count( kDirectoriesOptionLongName ) )
	{
		removeFolder = true;
	}
	
	if( vm.count( kFilesOptionLongName ) )
	{
		removeUnitFile = true;
	}
	
	if( vm.count( kIgnoreOptionLongName ) )
	{
		removeSequences = false;
	}
	
	if( vm.count( kVerboseOptionLongName ) )
	{
		verbose = true;
	}

	if( vm.count( kFirstImageOptionLongName ) )
	{
		selectRange = true;
		firstImage  = vm[kFirstImageOptionLongName].as< std::ssize_t >();
	}

	if( vm.count( kLastImageOptionLongName ) )
	{
		selectRange = true;
		lastImage  = vm[kLastImageOptionLongName].as< std::ssize_t >();
	}

	if( vm.count( kFullRMPathOptionLongName ) )
	{
		removeUnitFile  = true;
		removeFolder    = true;
		removeSequences = true;
	}
	
	if( vm.count( kAllOptionLongName ) )
	{
		// add .* files
		removeDotFile = true;
	}
	
	if( vm.count( kPathOptionLongName ) )
	{
		printAbsolutePath = true;
	}
	
	// defines paths, but if no directory specify in command line, we add the current path
	if( vm.count( kInputDirOptionLongName ) )
	{
		paths = vm[kInputDirOptionLongName].as< std::vector<std::string> >();
	}
	else
	{
		TUTTLE_COUT( _color._error << "No sequence and/or directory are specified." << _color._std );
		return 1;
	}

	if (vm.count(kRecursiveOptionLongName))
	{
		recursiveListing = true;
	}
	// 	for(unsigned int i=0; i<filters.size(); i++)
	// 	TUTTLE_COUT("filters = " << filters.at(i));
	// 	TUTTLE_COUT("research mask = " << researchMask);
	// 	TUTTLE_COUT("options  mask = " << descriptionMask);

	try
	{
		std::vector<boost::filesystem::path> pathsNoRemoved;
		BOOST_FOREACH( bfs::path path, paths )
		{
//			TUTTLE_TCOUT( "path: "<< path );
			if( bfs::exists( path ) )
			{
				/*
				if( bfs::is_directory( path ) )
				{
					//TUTTLE_TCOUT( "is a directory" );
					if(recursiveListing)
					{
						for ( bfs::recursive_directory_iterator end, dir(path); dir != end; ++dir )
						{
							if( bfs::is_directory( *dir ) )
							{
								//TUTTLE_TCOUT( *dir );
								std::list<boost::shared_ptr<FileObject> > listing = fileObjectsInDir( (bfs::path)*dir, filters, researchMask, descriptionMask );
								removeFileObject( listing, pathsNoRemoved );
							}
						}
					}
					std::list<boost::shared_ptr<FileObject> > listing = fileObjectsInDir( (bfs::path)path, filters, researchMask, descriptionMask );
					removeFileObject( listing, pathsNoRemoved );
				}
				else
				{
					//TUTTLE_TCOUT( "is NOT a directory "<< path.branch_path() << " | "<< path.leaf() );
					filters.push_back( path.leaf().string() );
					std::list<boost::shared_ptr<FileObject> > listing = fileObjectsInDir( (bfs::path)path.branch_path(), filters, researchMask, descriptionMask );
					removeFileObject( listing, pathsNoRemoved );
				}*/
			}
			else
			{
//				TUTTLE_TCOUT( "not exist ...." );
				
				try
				{
					Items items = sequence::parser::browse( path.string().c_str(), recursiveListing );
					/*Sequence s(path.branch_path(), descriptionMask );
					s.initFromDetection( path.string(), Sequence::ePatternDefault );
					if( s.getNbFiles() )
					{
//						TUTTLE_TCOUT( s );
						removeSequence( s );
					}*/
				}
				catch(... )
				{
					TUTTLE_CERR ( _color._error << "Unrecognized pattern \"" << path << "\"" << _color._std );
				}
			}
		}
		// delete not empty folder the first time
		//removeFiles( pathsNoRemoved );
	}
	catch (bfs::filesystem_error &ex)
	{
		TUTTLE_COUT( _color._error << ex.what() << _color._std );
	}
	catch(... )
	{
		TUTTLE_CERR ( _color._error << boost::current_exception_diagnostic_information() << _color._std );
	}
	return 0;
}

