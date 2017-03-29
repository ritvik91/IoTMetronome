#include "metronome.hpp"

   void metronome::start_timing()
   {
       m_timer.reset();
       m_timer.start();
       m_timing = true;
       for(int i=0; i<beat_samples; i++)
           m_beats[i]=0;
   }

   void metronome::stop_timing()
   {
       m_timer.stop();
       m_timing = false;
   }

   void metronome::tap()
   {
       if(is_timing() == true)
       {
           if(m_beat_count<beat_samples)
               m_beats[m_beat_count++] = m_timer.read_ms();
           else
           {
               for(int i=0; i<beat_samples-1; i++)
                   m_beats[i] = m_beats[i+1];
               m_beats[beat_samples-1] = m_timer.read_ms();
           }
       }    
   }

   size_t metronome::get_bpm() const
   {
       size_t bpm=0;
       for(int i=0; i<beat_samples-1; i++)
           bpm += m_beats[i+1] - m_beats[i];
       bpm /= ((beat_samples-1)*1000);

    if(bpm!=0)
    bpm = 60/bpm;
       return bpm;
   }
