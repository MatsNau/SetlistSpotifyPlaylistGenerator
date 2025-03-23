# Setlist Spotify Playlist Generator

A desktop application that bridges the gap between setlist.fm and Spotify, allowing you to easily turn concert setlists into Spotify playlists.
![image](https://github.com/user-attachments/assets/39211893-aecb-48f9-997e-5029e14d2607)


## Features

- Search for setlists from setlist.fm using their unique ID
- View detailed information about concerts including venue, date, and the full setlist
- Automatically generate Spotify playlists from setlists with a single click
- Smart song matching that handles cover songs by searching for the original artist
- Clean, responsive user interface built with Dear ImGui

## Technical Details

This application is built with:

- C++ (C++17 standard)
- Dear ImGui for the user interface
- DirectX 11 for rendering
- libcurl for interacting with the Spotify API and teh setlist.fm API
- nlohmann/json for JSON parsing

## Getting Started

### Prerequisites

- Windows 10/11
- Visual C++ Redistributable 2022 (or later)
- Spotify account
- setlist.fm API key

### Setup

1. Download the latest release or build from source
2. Create a `accessData.json` file in the application directory with the following structure:
   ```json
   {
     "spotify": {
       "client_id": "YOUR_SPOTIFY_CLIENT_ID",
       "client_secret": "YOUR_SPOTIFY_CLIENT_SECRET",
       "redirect_uri": "http://localhost:8080"
     },
     "setlistfm": {
       "api_key": "YOUR_SETLISTFM_API_KEY"
     }
   }
   ```
3. To obtain the necessary credentials:
   - For Spotify: Create an app at [Spotify Developer Dashboard](https://developer.spotify.com/dashboard/)
   - For setlist.fm: Request an API key at [setlist.fm API](https://api.setlist.fm/)

### Usage

1. Launch the application
2. Enter a setlist ID from setlist.fm (can be found in the URL of a setlist page)
3. Click "Load Setlist" to retrieve the concert information
4. Once loaded, review the setlist and click "Create Spotify Playlist"
5. The application will search for all songs and create a new playlist in your Spotify account

## Building from Source

1. Clone the repository
2. Install dependencies using vcpkg:
   ```
   vcpkg install boost fmt nlohmann-json curl imgui[core,dx11-binding,win32-binding]
   ```
3. Open the solution in Visual Studio 2022
4. Build the solution (Release configuration recommended for deployment)

## Project Structure

- `/src` - Source code
  - `/App` - Application core
  - `/Services` - API service implementations
  - `/UI` - User interface components
  - `/Utils` - Utility functions
- `/external` - External dependencies

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- [setlist.fm](https://www.setlist.fm/) for providing the concert data API
- [Spotify](https://developer.spotify.com/) for their excellent music API
- [Dear ImGui](https://github.com/ocornut/imgui) for the immediate mode GUI library
- All the open source libraries that made this project possible

## Disclaimer

This application is not affiliated with Spotify or setlist.fm. It was created as a personal project to showcase integration of multiple APIs and C++ GUI development techniques.
