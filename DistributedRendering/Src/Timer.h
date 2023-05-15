//From https://github.com/yottaawesome/intro-to-dx12/blob/master/src/Common/GameTimer.h

#pragma once

namespace DR
{
	class Timer
	{
	public:
		Timer();
		float TotalTime() const;
		float DeltaTime() const;

		void Reset();
		void Start();
		void Stop();
		void Tick();

	private:
		double m_SecondsPerCount;
		double m_DeltaTime;

		__int64 m_BaseTime;
		__int64 m_PausedTime;
		__int64 m_StopTime;
		__int64 m_PrevTime;
		__int64 m_CurrTime;

		bool m_Stopped;
	};
}

