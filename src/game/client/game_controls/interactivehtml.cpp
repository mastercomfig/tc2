#include "cbase.h"
#include "interactivehtml.h"

std::string CInteractiveHTML::UrlCommandPrefix = "do:";

void CInteractiveHTML::AddCommandListener( std::string commandName, const std::function<void( const std::string& )>& func)
{
	m_commandFunctions.emplace(commandName, func);
}

void CInteractiveHTML::RemoveCommandListener( std::string commandName )
{
	m_commandFunctions.erase(commandName);
}

bool CInteractiveHTML::OnStartRequest(const char* url, const char* target, const char* pchPostData, bool bIsRedirect)
{
	std::string psURL{ url };

	if ( !psURL.rfind( UrlCommandPrefix, 0 ) )
	{
		std::string token;
		size_t nPosStart = 0;
		while ( size_t nPosEnd = psURL.find(";", nPosStart) )
		{
			token = psURL.substr(nPosStart, nPosEnd - nPosStart);

			if ( token.rfind( UrlCommandPrefix, 0 ) )
			{
				continue;
			}

			token = token.substr( UrlCommandPrefix.length() );

			std::string psCommand{ token };
			std::string psQuery{};
			size_t pos;
			if ( (pos = token.find('?')) != std::string::npos )
			{
				psCommand = token.substr( 0, pos );
				psQuery = token.substr( pos + 1 );
			}

			std::unordered_map<std::string, std::function<void(const std::string&)>>::iterator funcIter;
			if ( (funcIter = m_commandFunctions.find(psCommand)) != m_commandFunctions.end() )
			{
				funcIter->second( std::string{ psQuery } );
			}

			if (nPosEnd == std::string::npos)
			{
				break;
			}
			nPosStart = nPosEnd + 1;
		}

		return false;
	}
	
	return HTML::OnStartRequest(url, target, pchPostData, bIsRedirect);
}