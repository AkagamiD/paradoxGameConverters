﻿using Caliburn.Micro;
using Frontend.Core.Factories.TagReaders;
using System.Xml.Linq;

namespace Frontend.Core.Factories
{
    public class RequiredFileFactory : FactoryBase
    {
        private FileTagReader fileTagReader;

        /// <summary>
        /// Initializes a new instance of the RequiredFileFactory
        /// </summary>
        /// <param name="eventAggregator">The event aggregator</param>
        public RequiredFileFactory(IEventAggregator eventAggregator)
            : base(eventAggregator, "requiredFile")
        {
            this.fileTagReader = new FileTagReader(eventAggregator);
        }

        protected override T OnBuildElement<T>(XElement element)
        {
            return (T)this.fileTagReader.ReadFile(element);
        }
    }
}
